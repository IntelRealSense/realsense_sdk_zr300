cmake_minimum_required(VERSION 2.8.9)
project(realsense_playback)

#------------------------------------------------------------------------------------
#Include
include_directories(
    .
    ..
    include
    ${ROOT_DIR}/include/rs/core
)

#------------------------------------------------------------------------------------
#Source Files
set(SOURCE_FILES_BASE
    disk_read_base.cpp
    playback_context.cpp
    playback_device_impl.cpp
    rs_stream_impl.cpp
    disk_read.cpp
    include/disk_read.h
    include/rs_stream_impl.h
    include/disk_read_factory.h
    include/disk_read_base.h
    include/disk_read_interface.h
    include/playback_device_impl.h
    include/playback_device_interface.h
    ${ROOT_DIR}/include/rs/core/context.h
    ${ROOT_DIR}/include/rs/playback/playback_device.h
    ${ROOT_DIR}/include/rs/playback/playback_context.h
)

set(SOURCE_FILES_LINUX
    include/linux/v1/file_types.h
    include/linux/v1/disk_read.h
    include/linux/v1/conversions.h
    linux/v1/disk_read.cpp
)

set(SOURCE_FILES_WINDOWS
    include/windows/v10/conversions.h
    include/windows/v10/file_types.h
    include/windows/v10/disk_read.h
    windows/v10/conversions.cpp
    windows/v10/disk_read.cpp
)

set(SOURCE_FILES_FILE
    ${ROOT_DIR}/src/cameras/include/file.h
    ${ROOT_DIR}/src/cameras/include/linear_algebra.h
    ${ROOT_DIR}/src/cameras/include/file_types.h
)

#Building Library
add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES_BASE}
    ${SOURCE_FILES_LINUX}
    ${SOURCE_FILES_WINDOWS}
    ${SOURCE_FILES_FILE}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    realsense_compression
    realsense_log_utils
)

#------------------------------------------------------------------------------------
#Dependencies
add_dependencies(${PROJECT_NAME}
    realsense_compression
    realsense_log_utils
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

#------------------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
