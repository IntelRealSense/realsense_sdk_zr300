cmake_minimum_required(VERSION 2.8.9)
project(realsense_sdk)

if(CMAKE_USE_INTEL_COMPILER EQUAL 1)
MESSAGE("---------    Using Intel compiler------------")
include(CMakeForceCompiler)
CMAKE_FORCE_CXX_COMPILER(icpc "Intel C++ Compiler")
else()
MESSAGE("---------    Using Default compiler ---------")
endif()

#-------------------- Lets make in-source-builds forbidden --------
MACRO(MACRO_ENSURE_OUT_OF_SOURCE_BUILD MSG)
     STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}"
     "${CMAKE_BINARY_DIR}" insource)
     GET_FILENAME_COMPONENT(PARENTDIR ${CMAKE_SOURCE_DIR} PATH)
     STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}"
     "${PARENTDIR}" insourcesubdir)
    IF(insource OR insourcesubdir)
        file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_SOURCE_DIR}/CMakeFiles")
        MESSAGE("============================================================")
        MESSAGE("======    ")
        MESSAGE("======    ${MSG}")
        MESSAGE("======    ")
        MESSAGE("============================================================")
        MESSAGE(FATAL_ERROR "Error occured")
    ENDIF(insource OR insourcesubdir)
ENDMACRO(MACRO_ENSURE_OUT_OF_SOURCE_BUILD)

MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
    "${CMAKE_PROJECT_NAME} requires an out of source build."
)

add_definitions(-Wall)
add_definitions(-Wno-write-strings)
add_definitions(-Wno-comment)
add_definitions(-Wno-unknown-pragmas)
add_definitions(-Wno-unused-function)
add_definitions(-Wno-unused-variable)
add_definitions(-Wno-reorder)
add_definitions(-Werror)

option (run_tests "set run_tests to ON if build tests should be run, set to OFF to skip tests" ON)
option (enable_logging "set enable_logging to ON to enable logging support, set to OFF to disable logging" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")


set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})


#--------------------- Release or Debug Builds ----------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#Specifying directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR}/bin/debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ROOT_DIR}/lib/debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DIR}/lib/debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 ")
else()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR}/bin/release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ROOT_DIR}/lib/release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DIR}/lib/release)
endif()

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})


set(INCLUDE_DIR ${ROOT_DIR}/include)
include_directories(${INCLUDE_DIR})

set(THIRDPARTY_DIR ${ROOT_DIR}/3rdparty)
MESSAGE("==>  Found thirparty dir: ${THIRDPARTY_DIR}")

install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION include/ )

include(CMakeVersion)

set(LIBVERSION ${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_COMMIT_NUMBER} )
set(LIBSOVERSION ${SDK_VERSION_MAJOR})


add_subdirectory(src/utilities)
add_subdirectory(src/core)
add_subdirectory(src/tools)
add_subdirectory(src/cameras)

if (run_tests)
    add_subdirectory(test)
endif ()
