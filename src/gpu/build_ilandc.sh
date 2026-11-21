#!/bin/bash
# Build the iLand console version (ilandc) from this repository, for the CPU and (if nvcc is
# found) with the CUDA path:
#     build/cpu/ilandc   - CPU build
#     build/gpu/ilandc   - the same plus light competition and seed dispersal on an NVIDIA GPU
#
# Usage (from anywhere):  src/gpu/build_ilandc.sh [cpu|gpu|both]      (default: both)
#
# Needs make, a C++17 compiler, Qt 6.5-6.8 (Qt 6.12 is too new: moc fails on bite/biteagent.h)
# with the core, xml, sql (SQLite driver), qml and concurrent modules, and FreeImage; for the GPU
# build also the CUDA toolkit (nvcc, CUDA 12 or 13). Qt and FreeImage via pixi (https://pixi.sh):
#     pixi run --manifest-path src/gpu/pixi.toml src/gpu/build_ilandc.sh
# Set CUDA_DIR if nvcc is not in /usr/local/cuda, and CUDA_ARCH to build for one GPU generation
# only (e.g. CUDA_ARCH="-arch=sm_80" for A100; default: all major generations).
set -euo pipefail
variants=${1:-both}
root=$(cd "$(dirname "$0")/../.." && pwd)
jobs=$(nproc 2>/dev/null || echo 8)
qmake=$(command -v qmake6 || command -v qmake || true)
[ -n "$qmake" ] || { echo "qmake not found (install Qt 6, or use pixi with src/gpu/pixi.toml)"; exit 1; }

# extra include/library paths for conda/pixi environments (FreeImage, Qt)
extra=()
if [ -n "${CONDA_PREFIX:-}" ]; then
    extra=("INCLUDEPATH+=$CONDA_PREFIX/include" "LIBS+=-L$CONDA_PREFIX/lib -Wl,-rpath,$CONDA_PREFIX/lib")
fi

mkdir -p "$root/src/plugins/build/release" "$root/build"
(cd "$root/src/plugins/build/release" && "$qmake" ../../plugins.pro && make -j"$jobs" -s)

build() {   # build VARIANT [qmake args...]
    local variant=$1; shift
    mkdir -p "$root/src/ilandc/build/$variant" "$root/build/$variant"
    (cd "$root/src/ilandc/build/$variant" && "$qmake" ../../ilandc.pro "$@" "${extra[@]}" && make -j"$jobs" -s)
    cp "$root/src/ilandc/build/$variant/ilandc" "$root/build/$variant/ilandc"
    echo "built $root/build/$variant/ilandc"
}

if [ "$variants" = cpu ] || [ "$variants" = both ]; then
    build cpu
fi
if [ "$variants" = gpu ] || [ "$variants" = both ]; then
    cuda_dir=${CUDA_DIR:-${CUDA_HOME:-/usr/local/cuda}}
    if [ -x "$cuda_dir/bin/nvcc" ]; then
        args=(CONFIG+=cuda "CUDA_DIR=$cuda_dir")
        [ -n "${CUDA_ARCH:-}" ] && args+=("CUDA_ARCH=$CUDA_ARCH")
        build gpu "${args[@]}"
    else
        echo "nvcc not found in $cuda_dir: skipping the GPU build (set CUDA_DIR)"
        [ "$variants" = gpu ] && exit 1
    fi
fi
