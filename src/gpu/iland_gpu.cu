/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
********************************************************************************************/
/** CUDA kernels for iLand's light competition and seed dispersal. See iland_gpu.h.
  *
  * Compiled with --fmad=false so that float expressions are rounded like the CPU code
  * (no fused multiply-add). Results still differ from the CPU in the last bits where
  * sums or products are formed in a different order (atomics, parallel reductions),
  * and the light grid uses a complete height grid for every tree (the multithreaded CPU
  * code builds the height grid resource unit by resource unit, interleaved with
  * applying the patterns). Long distance seed dispersal uses the GPU's own random
  * numbers, so GPU runs are statistically equivalent to CPU runs, not bit-identical.
  */
#include "iland_gpu.h"

#include <cuda_runtime.h>
#include <curand_kernel.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

namespace ilandgpu {

// ------------------------------------------------------------------ state and helpers
namespace {
std::mutex gMutex;           // serialises GPU use (seed dispersal runs one thread per species)
int gState = -1;             // -1: not checked, 0: disabled, 1: enabled
std::string gDescription;
char gLastError[512] = "";

// persistent device buffers, grown as needed
struct DeviceBuffer {
    void *ptr = nullptr; size_t bytes = 0;
    bool reserve(size_t n) {
        if (n <= bytes) return true;
        if (ptr) cudaFree(ptr);
        ptr = nullptr; bytes = 0;
        if (cudaMalloc(&ptr, n) != cudaSuccess) return false;
        bytes = n; return true;
    }
};
DeviceBuffer dStampData, dStampInfo, dDistance, dTrees, dHeight, dStemMax, dLif, dLri;
DeviceBuffer dSource, dSeed, dKernel, dLdd;
int gDistanceX = 0, gDistanceY = 0;
cudaStream_t gStream = nullptr;

bool check(cudaError_t err, const char *what) {
    if (err == cudaSuccess) return true;
    snprintf(gLastError, sizeof(gLastError), "%s: %s", what, cudaGetErrorString(err));
    return false;
}
#define CHECK(call) do { if (!check((call), #call)) return false; } while (0)

// atomics for non-negative floats: the int ordering of IEEE floats >= 0 is the float ordering
__device__ inline void atomicMaxPositive(float *addr, float value) {
    atomicMax(reinterpret_cast<int *>(addr), __float_as_int(value));
}
__device__ inline void atomicMinPositive(float *addr, float value) {
    atomicMin(reinterpret_cast<int *>(addr), __float_as_int(value));
}
// multiplicative update (light influence patterns multiply into the grid)
__device__ inline void atomicMultiply(float *addr, float factor) {
    int *iaddr = reinterpret_cast<int *>(addr);
    int old = *iaddr, assumed;
    do {
        assumed = old;
        old = atomicCAS(iaddr, assumed, __float_as_int(__int_as_float(assumed) * factor));
    } while (assumed != old);
}

struct HGV { float height; unsigned int count; };   // layout of HeightGridValue (8 bytes)
static_assert(sizeof(HGV) == 8, "HeightGridValue layout");

__device__ inline bool hgValid(const HGV &v) { return !((v.count >> 16) & 1u); }
__device__ inline bool hgForestOutside(const HGV &v) { return (v.count >> 17) & 1u; }
__device__ inline bool hgRadiating(const HGV &v) { return (v.count >> 18) & 1u; }

// ------------------------------------------------------------------ light kernels
__global__ void kFill(float *grid, size_t n, float value) {
    size_t i = blockIdx.x * (size_t)blockDim.x + threadIdx.x;
    if (i < n) grid[i] = value;
}

// Model::initializeGrid(): pixels around "radiating" height cells get a reduced start value
__global__ void kRadiate(float *lif, int sx, int sy, const HGV *hg, int hx, int hy, int pxh) {
    int cell = blockIdx.x * blockDim.x + threadIdx.x;
    if (cell >= hx * hy || !hgRadiating(hg[cell])) return;
    const int max_dist = 7, px_offset = pxh / 2;
    const float step = 1.f / static_cast<float>(max_dist);
    int px = cell % hx, py = cell / hx;
    int ix_min = px * pxh - max_dist + px_offset, iy_min = py * pxh - max_dist + px_offset;
    int ix_c = ix_min + max_dist, iy_c = iy_min + max_dist;
    for (int y = iy_min; y <= iy_min + 2 * max_dist + 1; ++y)
        for (int x = ix_min; x <= ix_min + 2 * max_dist + 1; ++x) {
            if (x < 0 || y < 0 || x >= sx || y >= sy) continue;
            if (!hgValid(hg[(y / pxh) * hx + x / pxh])) continue;
            float value = max(abs(x - ix_c), abs(y - iy_c)) * step;
            if (value >= 0.f) atomicMinPositive(&lif[(size_t)y * sx + x], value);
        }
}

// Tree::heightGrid()
__global__ void kHeight(const TreeLight *trees, int n, const StampInfo *stamps, HGV *hg, float *stem_max,
                        int hx, int hy, int pxh) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    TreeLight t = trees[i];
    int px = t.x / pxh, py = t.y / pxh;
    int cell = py * hx + px;
    atomicAdd(&hg[cell].count, 1u);
    atomicMaxPositive(&hg[cell].height, t.height);
    atomicMaxPositive(&stem_max[cell], t.height);
    int r = stamps[t.reader].offset;
    int iew = t.x % pxh, ins = t.y % pxh;
    if (iew - r < 0 && px > 0) atomicMaxPositive(&hg[cell - 1].height, t.height);
    if (iew + r >= pxh && px + 1 < hx) atomicMaxPositive(&hg[cell + 1].height, t.height);
    if (ins - r < 0 && py > 0) atomicMaxPositive(&hg[cell - hx].height, t.height);
    if (ins + r >= pxh && py + 1 < hy) atomicMaxPositive(&hg[cell + hx].height, t.height);
}

// stem height: the CPU keeps round(max height of trees on the cell) in bits 20-27
__global__ void kStemHeight(HGV *hg, const float *stem_max, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n || !(stem_max[i] > 0.f)) return;
    unsigned int hval = static_cast<unsigned int>(stem_max[i] + 0.5f);
    hg[i].count = (hg[i].count & 0xF00FFFFFu) | (hval << 20);
}

// Tree::applyLIP(): one block per tree, threads loop over the stamp's pixels
__global__ void kApply(const TreeLight *trees, int n, const StampInfo *stamps, const float *sdata,
                       const float *dist, int dx, const HGV *hg, int hx, float *lif, int sx, int sy, int pxh) {
    int i = blockIdx.x;
    if (i >= n) return;
    TreeLight t = trees[i];
    StampInfo w = stamps[t.writer];
    int size = 2 * w.offset + 1;
    int x0 = t.x - w.offset, y0 = t.y - w.offset;
    if (x0 < 0 || y0 < 0 || x0 >= sx || y0 >= sy || x0 + size < 0 || y0 + size < 0 || x0 + size >= sx || y0 + size >= sy)
        return; // as on the CPU: should not happen because of the buffer
    for (int k = threadIdx.x; k < size * size; k += blockDim.x) {
        int x = k % size, y = k / size;
        int gx = x0 + x, gy = y0 + y;
        float value = sdata[w.dataOffset + y * w.dataSize + x];
        float local_dom = hg[(gy / pxh) * hx + gx / pxh].height;
        float z = fmaxf(t.height - dist[abs(y - w.offset) * dx + abs(x - w.offset)], 0.f);
        float z_zstar = (z >= local_dom) ? 1.f : z / local_dom;
        value = 1.f - value * t.opacity * z_zstar;
        value = fmaxf(value, 0.02f);
        atomicMultiply(&lif[(size_t)gy * sx + gx], value);
    }
}

// Tree::readLIF() up to the raw sum: one block per tree, double-precision block reduction
__global__ void kRead(const TreeLight *trees, int n, const StampInfo *stamps, const float *sdata,
                      const float *dist, int dx, const HGV *hg, int hx, const float *lif, int sx, int sy,
                      int pxh, float *lri_raw) {
    __shared__ double partial[128];
    int i = blockIdx.x;
    if (i >= n) return;
    TreeLight t = trees[i];
    StampInfo w = stamps[t.writer], r = stamps[t.reader];
    int rsize = 2 * r.offset + 1;
    int d_offset = w.offset - r.offset;
    int rx = t.x - r.offset, ry = t.y - r.offset;
    double sum = 0.;
    for (int k = threadIdx.x; k < rsize * rsize; k += blockDim.x) {
        int x = k % rsize, y = k / rsize;
        int gx = rx + x, gy = ry + y;
        if (gx < 0 || gy < 0 || gx >= sx || gy >= sy) continue;
        HGV hgv = hg[(gy / pxh) * hx + gx / pxh];
        float local_dom = hgv.height;
        float z = fmaxf(t.height - dist[abs(y - r.offset) * dx + abs(x - r.offset)], 0.f);
        float z_zstar = (z >= local_dom) ? 1.f : z / local_dom;
        float prod = sdata[w.dataOffset + (y + d_offset) * w.dataSize + (x + d_offset)] * t.opacity * z_zstar;
        double own_value = 1. - prod;
        own_value = own_value > 0.02 ? own_value : 0.02;
        double value = lif[(size_t)gy * sx + gx] / own_value;
        if (hgForestOutside(hgv)) value *= 0.1f;
        sum += value * sdata[r.dataOffset + y * r.dataSize + x];
    }
    partial[threadIdx.x] = sum;
    __syncthreads();
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (threadIdx.x < s) partial[threadIdx.x] += partial[threadIdx.x + s];
        __syncthreads();
    }
    if (threadIdx.x == 0) lri_raw[i] = static_cast<float>(partial[0]);
}

// ------------------------------------------------------------------ seed kernels
// seed_map[t] += sum over kernel of source * kernel (the CPU scatters from sources; this gathers)
__global__ void kSeedConvolve(const float *src, float *seed, int sx, int sy, const float *kernel, int kx, int ky) {
    int x = blockIdx.x * blockDim.x + threadIdx.x, y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= sx || y >= sy) return;
    int offset = kx / 2;   // as SeedDispersal: offset = kernel.sizeX()/2 in both directions
    float acc = 0.f;
    for (int iy = 0; iy < ky; ++iy) {
        int syy = y + offset - iy;
        if (syy < 0 || syy >= sy) continue;
        for (int ix = 0; ix < kx; ++ix) {
            int sxx = x + offset - ix;
            if (sxx < 0 || sxx >= sx) continue;
            float s = src[syy * sx + sxx];
            if (s > 0.f) acc += s * kernel[iy * kx + ix];
        }
    }
    seed[y * sx + x] += acc;
}

// long distance dispersal: from every source pixel > 0, as in SeedDispersal::distributeSeeds()
__global__ void kSeedLDD(const float *src, float *seed, int sx, int sy, float cellsize, const float *density,
                         const float *distance, int n_rings, float ldd_value, unsigned long long rng_seed) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= sx * sy || !(src[i] > 0.f)) return;
    curandStatePhilox4_32_10_t st;
    curand_init(rng_seed, i, 0, &st);
    int px = i % sx, py = i / sx;
    for (int r = 0; r < n_rings; ++r) {
        int nld = density[r] < 1.f ? (curand_uniform(&st) < density[r] ? 1 : 0) : static_cast<int>(roundf(density[r]));
        for (int k = 0; k < nld; ++k) {
            float radius = (distance[r] + (distance[r + 1] - distance[r]) * curand_uniform(&st)) / cellsize;
            float phi = curand_uniform(&st) * 2.f * 3.14159265358979f;
            float sn, cs;
            sincosf(phi, &sn, &cs);
            int lx = px + static_cast<int>(radius * cs), ly = py + static_cast<int>(radius * sn);
            if (lx >= 0 && ly >= 0 && lx < sx && ly < sy) atomicAdd(&seed[ly * sx + lx], ldd_value);
        }
    }
}

__global__ void kSeedFinish(float *seed, int n, float fec) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n && seed[i] > 0.f) seed[i] = fminf(seed[i] * fec / 100.f, 1.f);
}

inline int blocks(size_t n, int threads) { return static_cast<int>((n + threads - 1) / threads); }
} // anonymous namespace

// ------------------------------------------------------------------ public API
bool enabled() {
    std::lock_guard<std::mutex> lock(gMutex);
    if (gState >= 0) return gState == 1;
    const char *env = std::getenv("ILAND_GPU");
    if (env && std::strcmp(env, "0") == 0) {
        gState = 0; gDescription = "GPU disabled (ILAND_GPU=0)"; return false;
    }
    int n = 0;
    if (cudaGetDeviceCount(&n) != cudaSuccess || n == 0) {
        gState = 0; gDescription = "no CUDA device found: running on the CPU"; return false;
    }
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    cudaSetDevice(0);
    cudaStreamCreateWithFlags(&gStream, cudaStreamNonBlocking);
    gDescription = std::string("GPU: ") + prop.name + " (light competition and seed dispersal on the GPU)";
    gState = 1;
    return true;
}

std::string description() { enabled(); return gDescription; }
const char *lastError() { return gLastError; }

void pinHostMemory(void *ptr, size_t bytes) {
    if (cudaHostRegister(ptr, bytes, cudaHostRegisterDefault) != cudaSuccess)
        cudaGetLastError(); // not fatal: transfers just stay pageable
}

void lightSetStamps(const float *data, int n_floats, const StampInfo *stamps, int n_stamps,
                    const float *distance_grid, int distance_size_x, int distance_size_y) {
    std::lock_guard<std::mutex> lock(gMutex);
    if (dStampData.reserve(n_floats * sizeof(float)))
        cudaMemcpy(dStampData.ptr, data, n_floats * sizeof(float), cudaMemcpyHostToDevice);
    if (dStampInfo.reserve(n_stamps * sizeof(StampInfo)))
        cudaMemcpy(dStampInfo.ptr, stamps, n_stamps * sizeof(StampInfo), cudaMemcpyHostToDevice);
    size_t nd = (size_t)distance_size_x * distance_size_y;
    if (dDistance.reserve(nd * sizeof(float)))
        cudaMemcpy(dDistance.ptr, distance_grid, nd * sizeof(float), cudaMemcpyHostToDevice);
    gDistanceX = distance_size_x; gDistanceY = distance_size_y;
}

bool lightRun(const TreeLight *trees, int n_trees, void *height_grid, int h_size_x, int h_size_y,
              float *lif_grid, int size_x, int size_y, int px_per_height, float *lri_raw) {
    std::lock_guard<std::mutex> lock(gMutex);
    size_t n_lif = (size_t)size_x * size_y, n_h = (size_t)h_size_x * h_size_y;
    if (!dTrees.reserve(std::max(1, n_trees) * sizeof(TreeLight)) || !dHeight.reserve(n_h * sizeof(HGV)) ||
        !dStemMax.reserve(n_h * sizeof(float)) || !dLif.reserve(n_lif * sizeof(float)) ||
        !dLri.reserve(std::max(1, n_trees) * sizeof(float))) {
        snprintf(gLastError, sizeof(gLastError), "out of GPU memory");
        return false;
    }
    auto *d_trees = static_cast<TreeLight *>(dTrees.ptr);
    auto *d_hg = static_cast<HGV *>(dHeight.ptr);
    auto *d_stem = static_cast<float *>(dStemMax.ptr);
    auto *d_lif = static_cast<float *>(dLif.ptr);
    auto *d_lri = static_cast<float *>(dLri.ptr);
    auto *d_stamps = static_cast<const StampInfo *>(dStampInfo.ptr);
    auto *d_sdata = static_cast<const float *>(dStampData.ptr);
    auto *d_dist = static_cast<const float *>(dDistance.ptr);
    cudaStream_t s = gStream;

    CHECK(cudaMemcpyAsync(d_trees, trees, n_trees * sizeof(TreeLight), cudaMemcpyHostToDevice, s));
    CHECK(cudaMemcpyAsync(d_hg, height_grid, n_h * sizeof(HGV), cudaMemcpyHostToDevice, s));
    CHECK(cudaMemsetAsync(d_stem, 0, n_h * sizeof(float), s));
    kFill<<<blocks(n_lif, 256), 256, 0, s>>>(d_lif, n_lif, 1.f);
    kRadiate<<<blocks(n_h, 128), 128, 0, s>>>(d_lif, size_x, size_y, d_hg, h_size_x, h_size_y, px_per_height);
    if (n_trees > 0) {
        kHeight<<<blocks(n_trees, 256), 256, 0, s>>>(d_trees, n_trees, d_stamps, d_hg, d_stem, h_size_x, h_size_y, px_per_height);
        kStemHeight<<<blocks(n_h, 256), 256, 0, s>>>(d_hg, d_stem, static_cast<int>(n_h));
        kApply<<<n_trees, 128, 0, s>>>(d_trees, n_trees, d_stamps, d_sdata, d_dist, gDistanceX, d_hg, h_size_x,
                                        d_lif, size_x, size_y, px_per_height);
        kRead<<<n_trees, 128, 0, s>>>(d_trees, n_trees, d_stamps, d_sdata, d_dist, gDistanceX, d_hg, h_size_x,
                                       d_lif, size_x, size_y, px_per_height, d_lri);
    }
    CHECK(cudaGetLastError());
    CHECK(cudaMemcpyAsync(lif_grid, d_lif, n_lif * sizeof(float), cudaMemcpyDeviceToHost, s));
    CHECK(cudaMemcpyAsync(height_grid, d_hg, n_h * sizeof(HGV), cudaMemcpyDeviceToHost, s));
    if (n_trees > 0)
        CHECK(cudaMemcpyAsync(lri_raw, d_lri, n_trees * sizeof(float), cudaMemcpyDeviceToHost, s));
    CHECK(cudaStreamSynchronize(s));
    return true;
}

bool seedDispersal(const float *source, float *seed_map, int size_x, int size_y, float cellsize,
                   const float *kernel, int kernel_size_x, int kernel_size_y,
                   float fec, const float *ldd_density, const float *ldd_distance, int n_rings,
                   float ldd_value, uint64_t seed) {
    std::lock_guard<std::mutex> lock(gMutex);
    size_t n = (size_t)size_x * size_y, nk = (size_t)kernel_size_x * kernel_size_y;
    if (!dSource.reserve(n * sizeof(float)) || !dSeed.reserve(n * sizeof(float)) ||
        !dKernel.reserve(nk * sizeof(float)) || !dLdd.reserve((2 * n_rings + 2) * sizeof(float))) {
        snprintf(gLastError, sizeof(gLastError), "out of GPU memory");
        return false;
    }
    auto *d_src = static_cast<float *>(dSource.ptr);
    auto *d_seed = static_cast<float *>(dSeed.ptr);
    auto *d_kernel = static_cast<float *>(dKernel.ptr);
    auto *d_ldd = static_cast<float *>(dLdd.ptr);
    cudaStream_t s = gStream;
    CHECK(cudaMemcpyAsync(d_src, source, n * sizeof(float), cudaMemcpyHostToDevice, s));
    CHECK(cudaMemcpyAsync(d_seed, seed_map, n * sizeof(float), cudaMemcpyHostToDevice, s));
    CHECK(cudaMemcpyAsync(d_kernel, kernel, nk * sizeof(float), cudaMemcpyHostToDevice, s));
    if (n_rings > 0) {
        CHECK(cudaMemcpyAsync(d_ldd, ldd_density, n_rings * sizeof(float), cudaMemcpyHostToDevice, s));
        CHECK(cudaMemcpyAsync(d_ldd + n_rings, ldd_distance, (n_rings + 1) * sizeof(float), cudaMemcpyHostToDevice, s));
    }
    dim3 tb(16, 16), grid(blocks(size_x, 16), blocks(size_y, 16));
    kSeedConvolve<<<grid, tb, 0, s>>>(d_src, d_seed, size_x, size_y, d_kernel, kernel_size_x, kernel_size_y);
    if (n_rings > 0)
        kSeedLDD<<<blocks(n, 128), 128, 0, s>>>(d_src, d_seed, size_x, size_y, cellsize, d_ldd, d_ldd + n_rings,
                                                 n_rings, ldd_value, seed);
    kSeedFinish<<<blocks(n, 256), 256, 0, s>>>(d_seed, static_cast<int>(n), fec);
    CHECK(cudaGetLastError());
    CHECK(cudaMemcpyAsync(seed_map, d_seed, n * sizeof(float), cudaMemcpyDeviceToHost, s));
    CHECK(cudaStreamSynchronize(s));
    return true;
}

} // namespace ilandgpu
