cmake_minimum_required(VERSION 2.8)
project(rs_video_module_async_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} video_module_async_sample.cpp)

target_link_libraries(${PROJECT_NAME}  rs_max_depth_value_module
                                       rs_image
                                       rs_playback
                                       rs_projection
                                       rs_log_utils
                                       realsense
                                       pthread
                                       opencv_imgproc opencv_core)

install(TARGETS ${PROJECT_NAME}  DESTINATION bin)
