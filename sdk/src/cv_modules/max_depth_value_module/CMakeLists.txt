cmake_minimum_required(VERSION 2.8.9)
project(realsense_max_depth_value_module)

set(SOURCE_FILES max_depth_value_module_impl.h
                 max_depth_value_module_impl.cpp
                 max_depth_value_module.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} realsense_log_utils ${SHLWAPI})

add_dependencies(${PROJECT_NAME} realsense_log_utils)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
