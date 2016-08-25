cmake_minimum_required(VERSION 2.8)
include(ExternalProject)
project(rs_tests)

set(SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sdk")
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

#Common settings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(-Wall)
add_definitions(-Wno-write-strings)
add_definitions(-Wno-comment)
add_definitions(-Wno-unknown-pragmas)
add_definitions(-Wno-unused-function)
add_definitions(-Wno-unused-variable)
add_definitions(-Wno-reorder)
add_definitions(-Werror)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 ")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 ")
endif()
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

#--------------Add security options --------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 ")  #TODO: Check what it is
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wformat -Wformat-security")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIE -fPIC")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -z noexecstack")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -z relro -z now")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -pie")


#Add Gtest project
ExternalProject_Add(
    gtest_lib
    URL http://googletest.googlecode.com/files/gtest-1.7.0.zip
    URL_HASH SHA1=f85f6d2481e2c6c4a18539e391aa4ea8ab0394af
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
               -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
               -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)
# Set gtest properties
ExternalProject_Get_Property(gtest_lib source_dir binary_dir)
include_directories("${source_dir}/include")
link_directories("${binary_dir}")

#build unit tests
include_directories(
    ${SDK_DIR}
    ${SDK_DIR}/include/rs/core
    ${SDK_DIR}/src/cameras/include
    ${SDK_DIR}/src/cameras/playback/include
    ${SDK_DIR}/src/cameras/record/include
    ${SDK_DIR}/src/core/image
    ${SDK_DIR}/src/utilities
    ${SDK_DIR}/src/utilities/logger/include
)

add_executable(${PROJECT_NAME}
    main.cpp
    projection_fixture.h
    utilities/utilities.h
    simple_streaming_tests.cpp
    playback_device_tests.cpp
    record_device_tests.cpp
    image_tests.cpp
    projection_tests.cpp
    librealsense_conversion_tests.cpp
    fps_counter_tests.cpp
    ${SDK_DIR}/include/rs/core/ref_count_interface.h
    ref_count_tests.cpp
    samples_time_sync_tests.cpp
)

target_link_libraries(${PROJECT_NAME}
    gtest
    gtest_main
    pthread
    realsense
    rs_image
    rs_playback
    rs_record
    rs_log_utils
    rs_viewer
    rs_samples_time_sync
)

add_dependencies(${PROJECT_NAME}
    rs_viewer
    rs_playback
    rs_record
    rs_log_utils
    rs_image
    rs_projection
    rs_samples_time_sync
    gtest_lib
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
