#!/bin/bash
if [ $1 = "Release" ]; then
    mkdir -p build/release
    cd build/release
elif [ $1 = "Debug" ]; then
    mkdir -p build/debug
    cd build/debug
fi
export CMAKE_PREFIX_PATH=$5;$CMAKE_PREFIX_PATH
cmake ../.. -DCMAKE_BUILD_TYPE=$1 -DPython3_INCLUDE_DIRS=$2 -DPython3_LIBRARY_DIRS=$3 -DPython3_Numpy_INCLUDE_DIRS=$4
rm  -r ../../ASRCAISim1/include/ASRCAISim1/
rm -r ../../ASRCAISim1/include/thirdParty/
make -j8
make install
