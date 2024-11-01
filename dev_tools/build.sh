#!/bin/bash

# TARGET_BUILD_TYPE=Debug
# TARGET_BUILD_TYPE=Release
TARGET_BUILD_TYPE=RelWithDebInfo

CUR_PYVER=$(echo "$(python --version)" | cut -d'.' -f2)
y=$(cat build/pyver 2>/dev/null)
if [[ $CUR_PYVER != $y ]]; then
    echo "py ver mismatch, removing build dir"
    rm -rf build
fi

# 定义一个函数来提取CMAKE_BUILD_TYPE的值
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

    # 返回结果
    echo "提取到的值是: '$build_type'"
}

# 调用函数
get_cmake_build_type

mkdir -p build
echo $CUR_PYVER > build/pyver
cd build

set -e

cmake .. -DCMAKE_BUILD_TYPE=$TARGET_BUILD_TYPE
cmake --build . --config $TARGET_BUILD_TYPE
