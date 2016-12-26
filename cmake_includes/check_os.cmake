# This file might be processed several times in certain builds.
# It is included several times since different projects need to compile as stand-alones and they all need to have some
#  or all of the definitions set (or included) in this file

get_filename_component(CMAKE_INCLUDES_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)

## Define CMake macros to identify the current OS easily throughout the CMakeLists files
## And include cmake according to current operating system

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(IS_CURRENT_SYSTEM_LINUX_OS ON)
    include(${CMAKE_INCLUDES_DIR}/config_linux.cmake)
endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(IS_CURRENT_SYSTEM_WINDOWS_OS ON)
    include(${CMAKE_INCLUDES_DIR}/config_windows.cmake)
endif()



