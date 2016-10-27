cmake_minimum_required(VERSION 2.8)
project(realsense_logger)

set(SOURCE_FILES logger.cpp xlevel.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} log4cxx apr-1 aprutil-1 )

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

