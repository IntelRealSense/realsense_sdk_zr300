cmake_minimum_required(VERSION 2.8.9)

project(realsense_image)

#------------------------------------------------------------------------------------
#Include
include_directories(
    .
    ..
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/utilities/image
)
#------------------------------------------------------------------------------------
#Source Files
set(SOURCE_FILES
    image_base.cpp
    image_base.h
    custom_image.cpp
    custom_image.h
    lrs_image.cpp
    lrs_image.h
    image_conversion_util.cpp
    image_conversion_util.h
    metadata.cpp
    metadata.h
    ${ROOT_DIR}/include/rs/utils/ref_count_base.h
    ${ROOT_DIR}/include/rs/core/image_interface.h
    ${ROOT_DIR}/include/rs/core/metadata_interface.h
)

#------------------------------------------------------------------------------------
#Flags
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-write-strings -g -O0")

#------------------------------------------------------------------------------------
#Building Library
add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    realsense_log_utils
    opencv_imgproc${OPENCV_VER} opencv_core${OPENCV_VER}
)

#------------------------------------------------------------------------------------
#Dependencies

add_dependencies(${PROJECT_NAME}
    realsense_log_utils
)


set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

#------------------------------------------------------------------------------------
