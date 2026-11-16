#!/bin/bash
set -e  # Exit on any error

QMAKE_BIN="${QMAKE:-qmake}"

mkdir -p src/plugins/build/release
mkdir -p src/iland/build/release
mkdir -p src/ilandc/build/release
mkdir -p src/fonstudio/build/release
mkdir -p src/tests/build/release

echo "Building plugins with $QMAKE_BIN..."
cd src/plugins/build/release
"$QMAKE_BIN" -r ../../plugins.pro
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building iland..."
cd src/iland/build/release
"$QMAKE_BIN" ../../iland.pro
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building ilandc..."
cd src/ilandc/build/release
"$QMAKE_BIN" ../../ilandc.pro
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building fonstudio..."
cd src/fonstudio/build/release
"$QMAKE_BIN" ../../fonstudio.pro
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
cd ../../../..

echo "Building and running unit tests..."
cd src/tests/build/release
qmake ../../tests.pro
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
./iland_tests
cd ../../../..

echo "==================================================================="
echo "Build Verification - Checking for built executables"
echo "==================================================================="
ls -la src/iland/build/release/
echo "---"
ls -la src/ilandc/build/release/
echo "---"
ls -la src/fonstudio/build/release/
