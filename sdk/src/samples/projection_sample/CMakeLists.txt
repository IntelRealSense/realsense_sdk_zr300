cmake_minimum_required(VERSION 2.8)
project(rs_projection_sample)

add_executable(${PROJECT_NAME} projection_sample.cpp)
target_link_libraries(${PROJECT_NAME} realsense rs_image rs_projection)
add_dependencies(${PROJECT_NAME} rs_image rs_projection)
