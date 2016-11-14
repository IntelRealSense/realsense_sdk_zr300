cmake_minimum_required(VERSION 2.8)
project(realsense_external_camera_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} main.cpp aux.h v4l2_streamer.h rs_streamer.h)
include_directories(include)

target_link_libraries(${PROJECT_NAME}   realsense
                                        realsense_viewer
                                        realsense_samples_time_sync
        )
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
