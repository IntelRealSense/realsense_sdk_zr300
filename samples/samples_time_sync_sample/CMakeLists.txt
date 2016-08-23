cmake_minimum_required(VERSION 2.8)
project(rs_samples_time_sync_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} samples_time_sync_sample.cpp)

target_link_libraries(${PROJECT_NAME} rs_image rs_samples_time_sync rs_log_utils realsense opencv_imgproc opencv_core)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
