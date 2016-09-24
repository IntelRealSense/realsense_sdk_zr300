cmake_minimum_required(VERSION 2.8)

project(realsense_compression)

include_directories(${ROOT_DIR}/src/cameras)

set(SOURCE_FILES
    compression_interface.h
    compression_mock.h
    compression_mock.cpp
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)




