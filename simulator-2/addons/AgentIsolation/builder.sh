#!/bin/bash
if [ $1 = "Release" ]; then
    mkdir -p build/release
    cd build/release
elif [ $1 = "Debug" ]; then
    mkdir -p build/debug
    cd build/debug
fi
export CMAKE_PREFIX_PATH=$5;$CMAKE_PREFIX_PATH
cmake ../.. -DADDON_NAME=$9 -DCMAKE_BUILD_TYPE=$1 -DPython3_INCLUDE_DIRS=$2 -DPython3_LIBRARY_DIRS=$3 -DPython3_Numpy_INCLUDE_DIRS=$4 -DCORE_SIMULATOR_DIR=$6 -DADDON_DST_DIR=$7 -DADDON_INCLUDE_DIR=$8
make -j8
make install
