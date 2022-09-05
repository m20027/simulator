set TARGET_NAME=Core
if  %1 == Release (
    mkdir build\release > NUL 2>&1
    cd build\release
) else if  %1 == Debug (
    mkdir build\debug > NUL 2>&1
    cd build/debug
)
if defined CMAKE_PREFIX_PATH (
    set CMAKE_PREFIX_PATH=%5;%CMAKE_PREFIX_PATH%
) else (
    set CMAKE_PREFIX_PATH=%5
)
cmake ..\.. -DCMAKE_BUILD_TYPE=%1 -DPython3_INCLUDE_DIRS=%2 -DPython3_LIBRARY_DIRS=%3 -DPython3_Numpy_INCLUDE_DIRS=%4
rd  /s /q ..\..\ASRCAISim1\include\ASRCAISim1\
rd /s /q ..\..\ASRCAISim1\include\thirdParty\
del ..\..\ASRCAISim1\%TARGET_NAME%.lib
del ..\..\ASRCAISim1\%TARGET_NAME%.dll
del ..\..\ASRCAISim1\lib%TARGET_NAME%.dll.a
del ..\..\ASRCAISim1\lib%TARGET_NAME%.dll
del ..\..\ASRCAISim1\lib%TARGET_NAME%.pyd
cmake --build . -j8 --config %1
cmake --install .