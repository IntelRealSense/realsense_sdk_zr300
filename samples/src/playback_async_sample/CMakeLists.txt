cmake_minimum_required(VERSION 2.8)
project(realsense_playback_async_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} realsense
                                      realsense_image
                                      realsense_playback
                                      opencv_imgproc
                                      opencv_core pthread)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

