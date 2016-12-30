cmake_minimum_required(VERSION 2.8)

project(realsense_compression)

set(SOURCE_FILES
    codec_interface.h
    lz4_codec.h
    lz4_codec.cpp
    encoder.h
    decoder.h
    encoder.cpp
    decoder.cpp
    encoder_synchronizer.h
    encoder_synchronizer.cpp
)

include_directories(
    ${ROOT_DIR}/src/include
    ${ROOT_DIR}/src/cameras
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE} ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

target_link_libraries(${PROJECT_NAME}
    ${LZ4}
    realsense_log_utils
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
