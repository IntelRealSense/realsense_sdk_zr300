cmake_minimum_required(VERSION 2.8)
project(rs_compression)

include_directories(${ROOT_DIR}/src/cameras)

set(SOURCE_FILES
    compression_interface.h
    compression_mock.h
    compression_mock.cpp
)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)




