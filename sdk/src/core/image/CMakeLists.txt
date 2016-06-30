cmake_minimum_required(VERSION 2.8.9)
project(rs_image)

#------------------------------------------------------------------------------------
#Include
include_directories(
    .
    ..
    ${ROOT_DIR}/include
    ${ROOT_DIR}/include/rs/core
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/utilities/image
)

#------------------------------------------------------------------------------------
#Source Files
set(SOURCE_FILES
    image_base.cpp
    ${ROOT_DIR}/include/rs/core/image_base.h
    custom_image.cpp
    ${ROOT_DIR}/include/rs/core/custom_image.h
    lrs_image.cpp
    ${ROOT_DIR}/include/rs/core/lrs_image.h
    image_conversion_util.cpp
    image_conversion_util.h
    metadata.cpp
    ${ROOT_DIR}/include/rs/core/metadata.h
    ${ROOT_DIR}/include/rs/core/image_interface.h
    ${ROOT_DIR}/include/rs/core/metadata_interface.h
)

#------------------------------------------------------------------------------------
#Flags
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-write-strings -g -O0")

#------------------------------------------------------------------------------------
#Building Library
add_library(${PROJECT_NAME} STATIC
    ${SOURCE_FILES}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
set(OPENCV_LIBS
        opencv_imgproc
        opencv_core
)

target_link_libraries(${PROJECT_NAME}
    ${OPENCV_LIBS}
    rs_log_utils
)

#------------------------------------------------------------------------------------
#Dependencies
add_dependencies(${PROJECT_NAME}
    rs_log_utils
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

#------------------------------------------------------------------------------------
