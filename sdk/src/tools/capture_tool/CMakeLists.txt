cmake_minimum_required(VERSION 2.8.9)
project(rs_capture_tool)

include_directories(
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
)

add_executable(${PROJECT_NAME}
    capture_tool.cpp
)

target_link_libraries(${PROJECT_NAME}
    realsense
    rs_record
    rs_playback
    rs_viewer
    rs_cl_util
)

add_dependencies(${PROJECT_NAME}
    rs_record
    rs_playback
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
