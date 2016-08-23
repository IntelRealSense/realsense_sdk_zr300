cmake_minimum_required(VERSION 2.8)
project(samples)

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

add_subdirectory(projection_sample)
add_subdirectory(record_sample)
add_subdirectory(playback_sample)
add_subdirectory(samples_time_sync_sample)
add_subdirectory(video_module_async_sample)
add_subdirectory(video_module_sync_sample)
add_subdirectory(fps_counter_sample)

