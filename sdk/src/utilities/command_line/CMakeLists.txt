cmake_minimum_required(VERSION 2.8)
project(rs_cl_util)

include_directories(
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/include
)

set(SOURCE_FILES
#    cmd_base.h
    cmd_base.cpp
#    basic_cmd_util.h
    basic_cmd_util.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
