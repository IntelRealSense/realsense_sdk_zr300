cmake_minimum_required(VERSION 2.8.9)
project(realsense_python)

find_package ( PythonInterp )
find_package ( PythonLibs )
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

#--------------Add security options --------------------
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 ")  #TODO: Check what it is
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-strong")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIE -fPIC")


message("PYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}")
message("PYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}")
message("PYTHON_INCLUDE_DIR:FILEPATH=${PYTHON_INCLUDE_DIR}")
message("PYTHONLIBS_VERSION_STRING=${PYTHONLIBS_VERSION_STRING}")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include(${SWIG_USE_FILE})


include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set_source_files_properties(playback.i PROPERTIES CPLUSPLUS ON)
swig_add_module(playback python playback.i)
swig_link_libraries(playback realsense_playback realsense  realsense_log_utils ${PYTHON_LIBRARIES} )

set_source_files_properties(streaming.i PROPERTIES CPLUSPLUS ON)
swig_add_module(streaming python streaming.i)
swig_link_libraries(streaming realsense pthread ${PYTHON_LIBRARIES})

set_source_files_properties(projection.i PROPERTIES CPLUSPLUS ON)
swig_add_module(projection python projection.i)
swig_link_libraries(projection realsense realsense_projection pthread ${PYTHON_LIBRARIES})

set_source_files_properties(image.i PROPERTIES CPLUSPLUS ON)
swig_add_module(image python image.i)
swig_link_libraries(image realsense realsense_image pthread ${PYTHON_LIBRARIES})
