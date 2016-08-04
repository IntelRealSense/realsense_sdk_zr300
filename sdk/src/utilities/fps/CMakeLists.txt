cmake_minimum_required(VERSION 2.8)
project(rs_fps_utils)

include_directories(
    include
    ${ROOT_DIR}/include/rs/core
)

add_library(${PROJECT_NAME} SHARED fps_utils.cpp)
