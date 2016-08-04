cmake_minimum_required(VERSION 2.8)
project(rs_viewer)

set(OPENCV_LIBRARIES opencv_core opencv_highgui opencv_imgproc)

include_directories(
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
)

set(SOURCE_FILES
    viewer.h
    viewer.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME}
    pthread
    rs_image
    glfw
    GL
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
