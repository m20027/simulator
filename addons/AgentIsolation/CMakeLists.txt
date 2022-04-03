cmake_minimum_required(VERSION 3.10)
project(${ADDON_NAME})
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
find_package(Eigen3 REQUIRED)
find_package(Boost REQUIRED)
find_package(pybind11 REQUIRED)
message(STATUS "Python Include:${Python3_INCLUDE_DIRS}")
message(STATUS "Python Libs:${Python3_LIBRARY_DIRS}")
message(STATUS "Numpy:${Python3_Numpy_INCLUDE_DIRS}")
message(STATUS "Eigen:${EIGEN3_INCLUDE_DIR}")
message(STATUS "Boost:${Boost_INCLUDE_DIRS}")
message(STATUS "Pybind11:${pybind11_INCLUDE_DIR}")
message(STATUS "Core Simulator Dir:${CORE_SIMULATOR_DIR}")
set(CMAKEPOSITION_INDEPENDENT_CODE True)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CORE_SIMULATOR_DIR}/include
    ${CORE_SIMULATOR_DIR}/include/thirdParty
    ${EIGEN3_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}/unsupported
    ${Boost_INCLUDE_DIRS}
    ${Python3_INCLUDE_DIRS}
    ${Python3_Numpy_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIR}
)
message(${CORE_SIMULATOR_DIR}/include)
link_directories(
    ${Python3_LIBRARY_DIRS}
	${CORE_SIMULATOR_DIR}
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
pybind11_add_module(${CMAKE_PROJECT_NAME} SHARED
    ${source}
)
if(WIN32)
    if(MSVC)
        set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
    else()
        set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "lib" SUFFIX ".dll")
    endif()
else()
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "lib" SUFFIX ".so")
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
	Core
)
if(NOT WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
        m
    )
endif()

install(
    TARGETS ${CMAKE_PROJECT_NAME}
		DESTINATION ${ADDON_DST_DIR}
)
install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/include/
	DESTINATION ${ADDON_INCLUDE_DIR}
)
install(
	DIRECTORY ${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/
	DESTINATION ${ADDON_DST_DIR}
)
