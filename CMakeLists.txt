cmake_minimum_required(VERSION 2.8.9)
project(realsense_sdk)

set(SDK_DIR sdk)
add_subdirectory(${SDK_DIR})
#samples built using this cmake will link to local sdk compilation
include_directories(${SDK_DIR}/include ${SDK_DIR}/src/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    link_directories(${SDK_DIR}/lib/debug)
else()
    link_directories(${SDK_DIR}/lib/release)
endif()

if(NOT WIN32)
add_subdirectory(samples)
endif(NOT WIN32)

option(BUILD_TESTS "set BUILD_TESTS to NO if build tests should be run, set to OFF to skip tests" OFF)
if(BUILD_TESTS)
 add_subdirectory(tests)
endif(BUILD_TESTS)


option(BUILD_DEB_PACKAGE "set BUILD_DEB_PACKAGE to NO if build debian package should be run, set to OFF to skip it" OFF)
if(BUILD_DEB_PACKAGE)
 include(debian/package_common)
endif(BUILD_DEB_PACKAGE)

