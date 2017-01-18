cmake_minimum_required(VERSION 2.8.9)
project(realsense_python)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
include(${ROOT_DIR}/cmake_includes/check_os.cmake)

find_package ( PythonInterp REQUIRED)
find_package ( PythonLibs REQUIRED)
find_package ( SWIG REQUIRED )

#--------------------- Release or Debug Builds ----------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#Specifying directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/debug)
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/debug)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 ")
else()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/release)
set(CMAKE_SWIG_OUTDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/release)
endif()


message("PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}")
message("PYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}")
message("PYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIR}")
message("PYTHONLIBS_VERSION_STRING=${PYTHONLIBS_VERSION_STRING}")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include(${SWIG_USE_FILE})


include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB MyFiles *.i)
set_property(SOURCE ${MyFiles} PROPERTY SWIG_FLAGS "-I/usr/local/include/")


set_source_files_properties(playback.i PROPERTIES CPLUSPLUS ON)
swig_add_module(playback python playback.i)
swig_link_libraries(playback realsense_playback realsense  realsense_log_utils ${PYTHON_LIBRARIES} )

set_source_files_properties(streaming.i PROPERTIES CPLUSPLUS ON)
swig_add_module(streaming python streaming.i)
swig_link_libraries(streaming realsense ${PTHREAD} ${PYTHON_LIBRARIES})

set_source_files_properties(projection.i PROPERTIES CPLUSPLUS ON)
swig_add_module(projection python projection.i)
swig_link_libraries(projection realsense realsense_image realsense_lrs_image realsense_projection ${PTHREAD} ${PYTHON_LIBRARIES})
