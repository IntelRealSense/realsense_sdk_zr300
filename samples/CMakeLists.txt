cmake_minimum_required(VERSION 2.8)
project(samples)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 ")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
endif()
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

#--------------Add security options --------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 ")  #TODO: Check what it is
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIE -fPIC")

add_subdirectory(src/projection_sample)
add_subdirectory(src/record_sample)
add_subdirectory(src/playback_sample)
add_subdirectory(src/samples_time_sync_sample)
add_subdirectory(src/video_module_async_sample)
add_subdirectory(src/video_module_sync_sample)
add_subdirectory(src/fps_counter_sample)
add_subdirectory(src/pipeline_async_sample)
add_subdirectory(src/external_camera_sample)
