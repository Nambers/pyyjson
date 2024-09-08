mkdir -p build
x=$(echo "$(python --version)" | cut -d'.' -f2)
y=$(cat build/pyver 2>/dev/null)
if [[ $x != $y ]]; then
    echo "py ver mismatch"
    rm -rf build
    mkdir build
fi
echo $x > build/pyver
cd build

# cmake .. -DCMAKE_BUILD_TYPE=Debug
# cmake --build . --config Debug
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
