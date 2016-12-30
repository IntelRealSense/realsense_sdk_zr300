
cmake_minimum_required(VERSION 2.8.9)
project(realsense_record)

#------------------------------------------------------------------------------------
#Include
include_directories(
    .
    ..
    include
    ${ROOT_DIR}/src/include
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
add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    realsense_compression
    realsense_log_utils
    realsense
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
