cmake_minimum_required(VERSION 2.8.9)
project(rs_playback)

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
    include/linux/disk_read_linux.h
    linux/disk_read_linux.cpp
)

set(SOURCE_FILES_WINDOWS
    include/windows/conversions.h
    include/windows/file_types_windows.h
    include/windows/disk_read_windows.h
    windows/disk_read_windows.cpp
)

set(SOURCE_FILES_FILE
    ${ROOT_DIR}/src/utilities/file/file.h
    ${ROOT_DIR}/src/cameras/include/file_types.h
)

#Building Library
add_library(${PROJECT_NAME}
    ${SOURCE_FILES_BASE}
    ${SOURCE_FILES_LINUX}
    ${SOURCE_FILES_WINDOWS}
    ${SOURCE_FILES_FILE}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    rs_compression
    rs_projection
    rs_log_utils
)

#------------------------------------------------------------------------------------
#Dependencies
add_dependencies(${PROJECT_NAME}
    rs_compression
    rs_projection
    rs_log_utils
)

#------------------------------------------------------------------------------------

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
