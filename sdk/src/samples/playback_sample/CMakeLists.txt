cmake_minimum_required(VERSION 2.8)
project(rs_playback_sample)

add_executable(${PROJECT_NAME} playback_sample.cpp)

target_link_libraries(${PROJECT_NAME} rs_image realsense rs_playback pthread)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
