cmake_minimum_required(VERSION 2.8.9)
project(realsense_sdk)

set(SDK_DIR sdk)
add_subdirectory(${SDK_DIR})

#samples built using this cmake will link to local sdk compilation
include_directories(${SDK_DIR}/include)
include_directories(${SDK_DIR}/src/include)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    link_directories(${SDK_DIR}/lib/debug)
else()
    link_directories(${SDK_DIR}/lib/release)
endif()

add_subdirectory(samples)

option(BUILD_TESTS "set run_tests to NO if build tests should be run, set to OFF to skip tests" OFF)
if(BUILD_TESTS)
	add_subdirectory(tests)
endif(BUILD_TESTS)
