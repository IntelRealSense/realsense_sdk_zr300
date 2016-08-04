cmake_minimum_required(VERSION 2.8.9)
project(rs_sync_utility)

#------------------------------------------------------------------------------------
#Include
include_directories(
    .
    ..
    include
    ${ROOT_DIR}/include/rs/core
)

#Source Files
set(SOURCE_FILES_BASE sync_utility.cpp)

#Building Library
add_library(${PROJECT_NAME} STATIC
    ${SOURCE_FILES_BASE}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    rs_log_utils
)

#------------------------------------------------------------------------------------
#Dependencies
add_dependencies(${PROJECT_NAME}
    rs_log_utils
)

#------------------------------------------------------------------------------------

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
