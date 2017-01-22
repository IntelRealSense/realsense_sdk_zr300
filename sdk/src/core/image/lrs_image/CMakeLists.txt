cmake_minimum_required(VERSION 2.8.9)

project(realsense_lrs_image)

#------------------------------------------------------------------------------------
#Include
include_directories(
    ..
    ${ROOT_DIR}/include
)
#------------------------------------------------------------------------------------
#Source Files
set(SOURCE_FILES
    lrs_image.cpp
    lrs_image.h
    ../image_base.h
    ../metadata.h
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    realsense
    realsense_image
    realsense_log_utils
    opencv_imgproc${OPENCV_VER} opencv_core${OPENCV_VER}
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
