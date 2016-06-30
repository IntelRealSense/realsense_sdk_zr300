cmake_minimum_required(VERSION 2.8)
project(cameras)

include_directories(${ROOT_DIR}/src/utilities)

add_subdirectory(compression)
add_subdirectory(record)
add_subdirectory(playback)
