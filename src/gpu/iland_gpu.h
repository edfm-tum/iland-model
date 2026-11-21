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
/** GPU (CUDA) kernels for the two most data-parallel parts of an iLand year:
  * the light competition (height grid, applying and reading light influence
  * patterns) and seed dispersal (kernel convolution plus long distance dispersal).
  *
  * This header is plain C++ (no Qt, no CUDA types), so the rest of iLand compiles
  * with an ordinary C++ compiler; only iland_gpu.cu needs nvcc. Built only with
  * qmake CONFIG+=cuda (which defines ILAND_CUDA). At runtime the GPU is used if a
  * CUDA device is present, unless the environment variable ILAND_GPU=0.
  */
#ifndef ILAND_GPU_H
#define ILAND_GPU_H

#include <cstdint>
#include <string>

namespace ilandgpu {

/// true if the GPU path is enabled (built with CUDA, a device is present, ILAND_GPU != 0)
bool enabled();
/// human readable description of the device (or why the GPU is not used)
std::string description();

// ------------------------------------------------------------------ light competition

/// one entry per light influence pattern (stamp); data is stored row-major with
/// 'dataSize' floats per row; 'offset' is the distance between edge and centre pixel.
struct StampInfo { int dataOffset; int dataSize; int offset; };

/// per-tree input for the light kernels (positions are indices on the 2m LIF grid)
struct TreeLight { int x; int y; float height; float opacity; int writer; int reader; };

/// upload all stamps (concatenated data) and the distance grid used by Stamp::distanceToCenter()
void lightSetStamps(const float *data, int n_floats, const StampInfo *stamps, int n_stamps,
                    const float *distance_grid, int distance_size_x, int distance_size_y);

/** run the light competition for one year:
  * 1. LIF grid := 1, plus the "radiating" border values (as Model::initializeGrid())
  * 2. height grid: tree count, dominant height (incl. crowns spilling into neighbours), stem height
  *    (as Tree::heightGrid())
  * 3. apply the light influence patterns multiplicatively (as Tree::applyLIP())
  * 4. read the light resource index of every tree (as Tree::readLIF(), before the
  *    relative-height correction, which needs the species' expression and runs on the CPU)
  * height_grid: the reset height grid (8 bytes per cell: float height + uint32 count/flags),
  *              overwritten with the result. lif_grid: output (size_x*size_y floats).
  * lri_raw: output, one value per tree. Returns false on a CUDA error (message in lastError()). */
bool lightRun(const TreeLight *trees, int n_trees,
              void *height_grid, int h_size_x, int h_size_y,
              float *lif_grid, int size_x, int size_y, int px_per_height,
              float *lri_raw);

/// pin a host buffer for faster transfers (e.g. the LIF grid); harmless if it fails
void pinHostMemory(void *ptr, size_t bytes);

// ------------------------------------------------------------------ seed dispersal

/** seed dispersal for one species and year (as SeedDispersal::distributeSeeds(), non-torus):
  * seed_map += convolution(source, kernel); plus long distance dispersal (LDD) from every
  * source pixel > 0 if n_rings > 0; then seed_map := min(seed_map * fec / 100, 1) where > 0.
  * source: already normalised (min(p / cellsize^2 / 3, 1)). LDD uses the GPU's own
  * counter-based random numbers (Philox), seeded with 'seed'.
  * ldd_density: n_rings values, ldd_distance: n_rings+1 values (metres). */
bool seedDispersal(const float *source, float *seed_map, int size_x, int size_y, float cellsize,
                   const float *kernel, int kernel_size_x, int kernel_size_y,
                   float fec, const float *ldd_density, const float *ldd_distance, int n_rings,
                   float ldd_value, uint64_t seed);

const char *lastError();

} // namespace ilandgpu

#endif // ILAND_GPU_H
