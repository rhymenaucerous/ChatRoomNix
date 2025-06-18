rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" ../
cmake --build . --config Debug
echo -e '\n\n'
cd ..
