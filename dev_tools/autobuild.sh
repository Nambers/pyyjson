#!/usr/bin/env -S bash
set -e
source ./dev_tools/get_env.sh
mkdir -p $BUILD_DIR
rm -rf ./$BUILD_DIR/*

if [ -z ${TARGET_BUILD_TYPE+x} ]; then
    TARGET_BUILD_TYPE=Release
fi

./.nix-pyenv/bin/cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$TARGET_BUILD_TYPE -DPython3_INCLUDE_DIR=$Python3_INCLUDE_DIR -DPython3_LIBRARY=$Python3_LIBRARY
cmake --build $BUILD_DIR

export PYTHONPATH=$(pwd)/$BUILD_DIR
if [ -z ${SKIP_TEST+x} ]; then
    if [ -z ${IGNORES+x} ]; then
        exec $Python3_EXECUTABLE test/all_test.py
    else
        exec $Python3_EXECUTABLE test/all_test.py --ignore $IGNORES
    fi
fi
