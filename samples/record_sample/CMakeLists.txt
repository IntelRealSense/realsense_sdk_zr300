cmake_minimum_required(VERSION 2.8)
project(rs_record_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} record_sample.cpp)

target_link_libraries(${PROJECT_NAME} rs_image rs_record rs_log_utils
                                      realsense pthread
                                      opencv_imgproc opencv_core)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
