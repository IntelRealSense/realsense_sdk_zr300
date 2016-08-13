cmake_minimum_required(VERSION 2.8.9)
project(rs_projection)

#Source Files
set(SOURCE_FILES
    projection_r200.cpp
    projection_r200.h
    ${ROOT_DIR}/include/rs/core/projection_interface.h
    math_projection_interface.h
    math_projection.cpp
)

#------------------------------------------------------------------------------------
#Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#------------------------------------------------------------------------------------
#Include
include_directories(
    .
    ..
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/core/image
    ${ROOT_DIR}/src/utilities/image
)

#------------------------------------------------------------------------------------
#Building Library
add_library(${PROJECT_NAME}
    ${SOURCE_FILES}
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
