#!/bin/bash

if [ $# -eq 0 ]; then
    set -e
    cd build
    cmake --build .
    exit 0
fi

TARGET_BUILD_TYPE=$1

CUR_PYVER=$(echo "$(python --version)" | cut -d'.' -f2)
y=$(cat build/pyver 2>/dev/null)
if [[ $CUR_PYVER != $y ]]; then
    echo "py ver mismatch, removing build dir"
    rm -rf build
fi

get_cmake_build_type() {
    if [ -f "./build/CMakeCache.txt" ]; then
        build_type=$(grep "CMAKE_BUILD_TYPE" ./build/CMakeCache.txt | cut -d'=' -f2)
    else
        build_type=""
    fi

    if [[ $build_type != "" && $TARGET_BUILD_TYPE != $build_type ]]; then
        echo "CMAKE_BUILD_TYPE mismatch, removing build dir"
        rm -rf build
    fi
}

get_cmake_build_type

mkdir -p build
echo $CUR_PYVER > build/pyver
cd build

set -e

cmake .. -DCMAKE_BUILD_TYPE=$TARGET_BUILD_TYPE
cmake --build . --config $TARGET_BUILD_TYPE
