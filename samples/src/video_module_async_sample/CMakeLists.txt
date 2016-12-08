cmake_minimum_required(VERSION 2.8)
project(realsense_video_module_async_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}  realsense_max_depth_value_module
                                       realsense_image
                                       realsense_lrs_image
                                       realsense_playback
                                       realsense_projection
                                       realsense
                                       pthread
                                       opencv_imgproc opencv_core)

install(TARGETS ${PROJECT_NAME}  DESTINATION bin)
