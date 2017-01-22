cmake_minimum_required(VERSION 2.8.9)

project(realsense_v4l_image)

include_directories(
    ${ROOT_DIR}/include/rs/core
)

set(SOURCE_FILES
    v4l_image_factory.cpp
    ${ROOT_DIR}/include/rs/core/v4l_image_factory.h
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
        realsense_image
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
