
cmake_minimum_required(VERSION 2.8.9)
project(rs_record)

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
set(SOURCE_FILES
    disk_write.cpp
    record_device_impl.cpp
    record_context.cpp
    include/disk_write.h
    include/record_device_impl.h
    include/record_device_interface.h
    ${ROOT_DIR}/src/cameras/include/file_types.h
    ${ROOT_DIR}/include/rs/record/record_device.h
    ${ROOT_DIR}/include/rs/record/record_context.h
)

#------------------------------------------------------------------------------------
#Building Library
add_library(${PROJECT_NAME} STATIC
    ${SOURCE_FILES}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    rs_compression
    rs_log_utils
)

#------------------------------------------------------------------------------------
#Dependencies
add_dependencies(${PROJECT_NAME}
    rs_compression
    rs_log_utils
)

#------------------------------------------------------------------------------------

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
