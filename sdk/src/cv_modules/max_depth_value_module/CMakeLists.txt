cmake_minimum_required(VERSION 2.8.9)
project(rs_max_depth_value_module)

set(SOURCE_FILES max_depth_value_module_impl.h
                 max_depth_value_module_impl.cpp
                 max_depth_value_module.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} rs_log_utils)

add_dependencies(${PROJECT_NAME} rs_log_utils)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
