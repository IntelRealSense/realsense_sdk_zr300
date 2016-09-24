cmake_minimum_required(VERSION 2.8.9)
project(rs_projection_tool)

add_definitions(${COMPILE_DEFINITIONS})

include_directories(
    include
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/include/rs/core
    ${ROOT_DIR}/src/cameras/playback/include
    ${ROOT_DIR}/src/utilities
    ${OpenCV_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    projection_tool.cpp
    projection_gui.cpp
)

target_link_libraries(${PROJECT_NAME}

    realsense
    realsense_image
    realsense_playback
    realsense_projection
    opencv_imgproc${OPENCV_VER} opencv_core${OPENCV_VER}
    opencv_highgui${OPENCV_VER}
    ${PTHREAD}


)

add_dependencies(${PROJECT_NAME}
    realsense_image
    realsense_playback
    realsense_projection
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
