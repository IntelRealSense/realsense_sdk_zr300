cmake_minimum_required(VERSION 2.8)
project(logger)

include_directories(
    include
    ${ROOT_DIR}/include/rs/core
)

set(LOGGER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(log_utils)

# Uncomment this file if you want to enable logging
# Logging will be enabled only if log4cxx is installed on your system
#
# add_subdirectory(logger)

