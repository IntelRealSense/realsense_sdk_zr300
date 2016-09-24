cmake_minimum_required(VERSION 2.8.9)
project(rs_capture_tool)

include_directories(
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/include
)

add_executable(${PROJECT_NAME}
    capture_tool.cpp
)

target_link_libraries(${PROJECT_NAME}
    realsense
    realsense_record
    realsense_playback
    realsense_viewer
    realsense_cl_util
    ${OPENGL_LIBS}
    ${GLFW_LIBS}
)

add_dependencies(${PROJECT_NAME}
    realsense_record
    realsense_playback
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
