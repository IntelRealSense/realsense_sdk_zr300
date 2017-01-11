cmake_minimum_required(VERSION 2.8.9)

project(realsense_image)
add_subdirectory(lrs_image)

include_directories(
    ${ROOT_DIR}/include
)

set(SOURCE_FILES
    image_base.cpp
    image_base.h
    custom_image.cpp
    custom_image.h
    image_conversion_util.cpp
    image_conversion_util.h
    metadata.cpp
    metadata.h
    ${ROOT_DIR}/include/rs/utils/ref_count_base.h
    ${ROOT_DIR}/include/rs/core/image_interface.h
    ${ROOT_DIR}/include/rs/core/metadata_interface.h
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    opencv_imgproc${OPENCV_VER} opencv_core${OPENCV_VER}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
