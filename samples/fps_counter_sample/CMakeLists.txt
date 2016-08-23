cmake_minimum_required(VERSION 2.8)
project(rs_fps_counter_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${PROJECT_NAME} fps_counter_sample.cpp)

target_link_libraries(${PROJECT_NAME} realsense)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
