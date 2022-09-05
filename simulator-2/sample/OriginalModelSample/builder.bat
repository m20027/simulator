set TARGET_NAME=OriginalModelSample
if  %1 == Release (
    mkdir build\release > NUL 2>&1
    cd build\release
)else if  %1 == Debug (
    mkdir build\debug > NUL 2>&1
    cd build/debug
)
if defined CMAKE_PREFIX_PATH (
    set CMAKE_PREFIX_PATH=%5;%CMAKE_PREFIX_PATH%
) else (
    set CMAKE_PREFIX_PATH=%5
)
cmake ..\.. -DCMAKE_BUILD_TYPE=%1 -DPython3_INCLUDE_DIRS=%2 -DPython3_LIBRARY_DIRS=%3  -DPython3_Numpy_INCLUDE_DIRS=%4 -DCORE_SIMULATOR_DIR=%6
rd  /s /q ..\..\%TARGET_NAME%\include\
del ..\..\%TARGET_NAME%\%TARGET_NAME%.lib
del ..\..\%TARGET_NAME%\%TARGET_NAME%.dll
del ..\..\%TARGET_NAME%\lib%TARGET_NAME%.dll.a
del ..\..\%TARGET_NAME%\lib%TARGET_NAME%.dll
del ..\..\%TARGET_NAME%\lib%TARGET_NAME%.pyd
cmake --build . -j8 --config %1
cmake --install .