cmake_minimum_required(VERSION 3.10)
project(ASRCAISim1)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
find_package(Eigen3 REQUIRED)
find_package(Boost REQUIRED)
find_package(pybind11 REQUIRED)
message(STATUS "Include:${Python3_INCLUDE_DIRS}")
message(STATUS "Libs:${Python3_LIBRARY_DIRS}")
message(STATUS "Numpy:${Python3_Numpy_INCLUDE_DIRS}")
message(STATUS "Eigen:${EIGEN3_INCLUDE_DIR}")
message(STATUS "Boost:${Boost_INCLUDE_DIRS}")
message(STATUS "Pybind11:${pybind11_INCLUDE_DIR}")
set(CMAKE_POSITION_INDEPENDENT_CODE True)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/thirdParty/include
    ${EIGEN3_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}/unsupported
    ${Boost_INCLUDE_DIRS}
    ${Python3_INCLUDE_DIRS}
    ${Python3_Numpy_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIR}
)
link_directories(
    ${Python3_LIBRARY_DIRS}
    ${PROJECT_SOURCE_DIR}/thirdParty/lib
)

add_definitions(
    -DEIGEN_MPL2_ONLY
    -D_USE_MATH_DEFINES
    -DHAVE_SNPRINTF
)
if(MSVC)
    add_compile_options("/utf-8")
    add_compile_options("/bigobj")
    add_compile_options("/D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING")
    add_definitions(
        -DNOMINMAX
    )
endif()
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src source)
pybind11_add_module(Core SHARED
    ${source}
)
if(WIN32)
    if(MSVC)
        set_target_properties(Core PROPERTIES PREFIX "" SUFFIX ".dll")
    else()
        set_target_properties(Core PROPERTIES PREFIX "lib" SUFFIX ".dll")
    endif()
else()
    set_target_properties(Core PROPERTIES PREFIX "lib" SUFFIX ".so")
endif()
target_link_libraries(Core PRIVATE
    nlopt
)
if(WIN32)
    target_link_libraries(Core PRIVATE
        bcrypt
    )
else()
    target_link_libraries(Core PRIVATE
        m
    )
endif()

install(
    TARGETS Core
    DESTINATION ASRCAISim1
)
install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/include/
	DESTINATION ${PROJECT_SOURCE_DIR}/ASRCAISim1/include/ASRCAISim1
)
install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/thirdParty/include/
	DESTINATION ${PROJECT_SOURCE_DIR}/ASRCAISim1/include/thirdParty
)