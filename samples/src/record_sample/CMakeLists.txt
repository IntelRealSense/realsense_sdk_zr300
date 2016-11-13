cmake_minimum_required(VERSION 2.8)
project(rs_record_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} cpp/main.cpp)

target_link_libraries(${PROJECT_NAME} realsense_image realsense_record realsense_log_utils
                                      realsense pthread
                                      opencv_imgproc opencv_core)


install(TARGETS ${PROJECT_NAME} DESTINATION bin)
