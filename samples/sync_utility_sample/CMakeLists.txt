cmake_minimum_required(VERSION 2.8)
project(rs_sync_utility_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} sync_utility_sample.cpp)

target_link_libraries(${PROJECT_NAME} rs_image rs_record rs_log_utils rs_sync_utility
                                      realsense pthread
                                      opencv_imgproc opencv_core)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
