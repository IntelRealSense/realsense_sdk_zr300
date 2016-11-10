cmake_minimum_required(VERSION 2.8)
project(rs_external_camera_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} cpp/main.cpp include/aux.h include/v4l2_streamer.h include/rs_streamer.h)
include_directories(include)

target_link_libraries(${PROJECT_NAME}   realsense
                                        realsense_viewer
        )
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
