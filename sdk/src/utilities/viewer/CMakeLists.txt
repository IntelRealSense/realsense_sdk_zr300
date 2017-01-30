cmake_minimum_required(VERSION 2.8)
project(realsense_viewer)

include_directories(
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/include

)

set(SOURCE_FILES
#    viewer.h
    viewer.cpp
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME}
    realsense_image
    ${PTHREAD}
    ${GLFW_LIBS}
    ${OPENGL_LIBS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
