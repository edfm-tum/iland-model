#!/bin/bash
set -e  # Exit on any error

mkdir -p src/plugins/build/release
mkdir -p src/iland/build/release
mkdir -p src/ilandc/build/release
mkdir -p src/fonstudio/build/release

echo "Building plugins..."
cd src/plugins/build/release
qmake ../../plugins.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building iland..."
cd src/iland/build/release
qmake ../../iland.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building ilandc..."
cd src/ilandc/build/release
qmake ../../ilandc.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building fonstudio..."
cd src/fonstudio/build/release
qmake ../../fonstudio.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "==================================================================="
echo "Build Verification - Checking for built executables"
echo "==================================================================="
ls -la src/iland/build/release/
echo "---"
ls -la src/ilandc/build/release/
echo "---"
ls -la src/fonstudio/build/release/
