#!/usr/bin/env -S bash
mkdir -p build
rm -rf ./build/*
SRC_ROOT=$(pwd)
cd build
set -e

CUR_PYVER=$(echo "$(python --version)" | cut -d'.' -f2)
if [[ $CUR_PYVER == "14" ]]; then
    $SRC_ROOT/.nix-pyenv/bin/cmake .. -DCMAKE_BUILD_TYPE=Debug -DASAN_ENABLED=on -DPython3_INCLUDE_DIR=$(python -c "from sysconfig import get_config_h_filename; from os.path import dirname; print(dirname(get_config_h_filename()))") -DPython3_EXECUTABLE=$(readlink -f $(which python)) -DPython3_LIBRARY=$(python -c "from sysconfig import get_config_var; print(get_config_var('LIBDIR'))")
else
    $SRC_ROOT/.nix-pyenv/bin/cmake .. -DCMAKE_BUILD_TYPE=Debug -DASAN_ENABLED=on
fi
cmake --build . --config Debug
cd ..
set +e
LD_PRELOAD=$SRC_ROOT/.nix-pyenv/lib/libasan.so PYTHONPATH=$SRC_ROOT/build $SRC_ROOT/.nix-pyenv/bin/python test/all_test.py --ignore bench
