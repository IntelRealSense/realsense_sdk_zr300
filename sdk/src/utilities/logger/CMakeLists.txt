cmake_minimum_required(VERSION 2.8)
project(logger)

include_directories(
    include
    ${ROOT_DIR}/include/rs/core
)


set(LOGGER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(log_utils)

if (enable_logging)
    add_subdirectory(logger)
endif ()
