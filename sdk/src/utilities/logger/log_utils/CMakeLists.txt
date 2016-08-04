cmake_minimum_required(VERSION 2.8)
project(rs_log_utils)

set(SOURCE_FILES log_utils.cpp
                 ${ROOT_DIR}/include/rs/utils/log_utils.h)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} dl)

MESSAGE("User home directory is: " $ENV{HOME})

#set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}")
#set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
