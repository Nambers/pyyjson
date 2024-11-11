#!/usr/bin/env -S bash
mkdir -p build
rm -rf ./build/*
SRC_ROOT=$(pwd)
cd build
set -e
$SRC_ROOT/.nix-pyenv/bin/cmake .. -DCMAKE_BUILD_TYPE=Debug -DASAN_ENABLED=on
cmake --build . --config Debug
cd ..
set +e
LD_PRELOAD=$SRC_ROOT/.nix-pyenv/lib/libasan.so PYTHONPATH=$SRC_ROOT/build $SRC_ROOT/.nix-pyenv/bin/python test/all_test.py
