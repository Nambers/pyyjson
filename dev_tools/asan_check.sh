#!/usr/bin/env -S bash
set -e
source ./dev_tools/get_env.sh
mkdir -p ./$BUILD_DIR
rm -rf ./$BUILD_DIR/*
./.nix-pyenv/bin/cmake . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -DASAN_ENABLED=on -DPython3_INCLUDE_DIR=$Python3_INCLUDE_DIR -DPython3_LIBRARY=$Python3_LIBRARY
cmake --build $BUILD_DIR --config Debug
export LD_PRELOAD=$(pwd)/.nix-pyenv/lib/libasan.so
export PYTHONPATH=$(pwd)/$BUILD_DIR
exec $Python3_EXECUTABLE test/all_test.py --ignore bench file
