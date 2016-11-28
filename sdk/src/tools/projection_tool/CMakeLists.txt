cmake_minimum_required(VERSION 2.8.9)
project(rs_projection_tool)

add_definitions(${COMPILE_DEFINITIONS})

include_directories(
    include
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/include
    ${ROOT_DIR}/include/rs/core
    ${ROOT_DIR}/src/cameras/playback/include
    ${ROOT_DIR}/src/utilities
    ${OpenCV_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    projection_cmd_util.h
    projection_viewer.cpp
    projection_tool.cpp
)

target_link_libraries(${PROJECT_NAME}

    realsense
    realsense_image
    realsense_playback
    realsense_projection
    realsense_viewer
    realsense_cl_util
    realsense_samples_time_sync
    ${PTHREAD}


)

add_dependencies(${PROJECT_NAME}
    realsense_image
    realsense_playback
    realsense_projection
    realsense_viewer
    realsense_cl_util
    realsense_samples_time_sync
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
