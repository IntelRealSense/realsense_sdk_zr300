cmake_minimum_required(VERSION 2.8.9)
project(realsense_sdk)

include(cmake_includes/check_os.cmake)

## Set preprocessors defines accroding to OS, to be used within header files

if( ${IS_CURRENT_SYSTEM_LINUX_OS})
    add_definitions(-DLINUX)
    MESSAGE("Added preprocessor definition: LINUX")
endif()


set(SDK_DIR sdk)
add_subdirectory(${SDK_DIR})

#samples built using this cmake will link to local sdk compilation
include_directories(${SDK_DIR}/include ${SDK_DIR}/src/include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    link_directories(${SDK_DIR}/lib/debug)
else()
    link_directories(${SDK_DIR}/lib/release)
endif()

if(${IS_CURRENT_SYSTEM_LINUX_OS})
    add_subdirectory(samples)
    option(BUILD_PKG_CONFIG "set BUILD_PKG_CONFIG to ON if pkg-config tool should be built, set to OFF to skip it" OFF)
    if(BUILD_PKG_CONFIG)
        include(cmake_includes/pkg_config)
    endif(BUILD_PKG_CONFIG)
endif(${IS_CURRENT_SYSTEM_LINUX_OS})

if(${IS_CURRENT_SYSTEM_WINDOWS_OS})
	option(WINDOWS_PKG "set WINDOWS_PKG to ON if build tests should be run, set to OFF to skip tests" OFF)
	if(WINDOWS_PKG)
		include(cmake_includes/pkg_windows.cmake)
	endif(WINDOWS_PKG)
endif(${IS_CURRENT_SYSTEM_WINDOWS_OS})

option(BUILD_TESTS "set BUILD_TESTS to ON if build tests should be run, set to OFF to skip tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)

option(BUILD_PYTHON_INTERFACE "set BUILD_PYTHON_INTERFACE to NO if build python build module should be run, set to OFF to skip it" OFF)
if(BUILD_PYTHON_INTERFACE)
    add_subdirectory(python)
endif(BUILD_PYTHON_INTERFACE)
