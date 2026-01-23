# build iLand on Linux
# release mode

mkdir src/plugins/build/release
mkdir src/iland/build/release
mkdir src/ilandc/build/release

cd src/plugins/build/release
qmake ../../plugins.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

cd ../../../..
cd src/iland/build/release
qmake ../../iland.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

cd ../../../..
cd src/ilandc/build/release
qmake ../../ilandc.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

echo "==================================================================="
echo "Executables in src/ilandc/build/release and src/iland/build/release"
echo "==================================================================="
