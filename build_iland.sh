# build iLand on Linux
# release mode

mkdir -p src/plugins/build/release
mkdir -p src/iland/build/release
mkdir -p src/ilandc/build/release
mkdir -p src/fonstudio/build/release

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

cd ../../../..
cd src/fonstudio/build/release
qmake ../../fonstudio.pro 
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)


echo "==================================================================="
echo "Executables in src/ilandc/build/release and src/iland/build/release"
echo "==================================================================="
