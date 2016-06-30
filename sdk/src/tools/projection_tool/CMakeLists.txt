cmake_minimum_required(VERSION 2.8.9)
project(rs_projection_tool)

add_definitions(-std=c++11 -Wno-write-strings)

#find_package(OpenCV REQUIRED)

set(OPENCV_LIBRARIES opencv_core opencv_highgui opencv_imgproc)

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
        rs_image
        rs_playback
        pthread
        rs_projection
        ${OPENCV_LIBRARIES}
)

add_dependencies(${PROJECT_NAME}
        rs_log_utils
        rs_projection
        rs_image
        rs_playback
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
