cmake_minimum_required(VERSION 2.8.9)
project(realsense_sdk)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
include(${ROOT_DIR}/cmake_includes/check_os.cmake)

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

add_definitions(${COMPILE_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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

#--------------Add security options --------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 ")  #TODO: Check what it is
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIE -fPIC")

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

set(INCLUDE_DIR ${ROOT_DIR}/include)
include_directories(${INCLUDE_DIR})

set(THIRDPARTY_DIR ${ROOT_DIR}/3rdparty)
MESSAGE("==>  Found thirparty dir: ${THIRDPARTY_DIR}")

install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION include/ )

include(CMakeVersion)

set(LIBVERSION ${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH})
set(LIBSOVERSION ${SDK_VERSION_MAJOR})

#----------- Build shared or static library ------------
option(BUILD_STATIC "Set to ON to build static libraries. Some libraries are always shared" OFF)
if(BUILD_STATIC)
    MESSAGE("Building libraries as static objects")
    set(SDK_LIB_TYPE "STATIC")
else()
    MESSAGE("Building libraries as shared objects(default)")
    set(SDK_LIB_TYPE "SHARED")
endif(BUILD_STATIC)


add_subdirectory(src/utilities)
add_subdirectory(src/core)
add_subdirectory(src/tools)
add_subdirectory(src/cameras)
add_subdirectory(src/cv_modules)
