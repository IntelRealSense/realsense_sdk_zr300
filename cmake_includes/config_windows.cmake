#if your current path of librealsense folder isn't C:/realsense/3rdparty/librealsense, please update it
if(NOT DEFINED LIBREALSENSE_DIR)
set(LIBREALSENSE_DIR "C:/realsense/3rdparty/librealsense")
endif(NOT DEFINED LIBREALSENSE_DIR)

set(LIBREALSENSE_INCLUDE_PATH ${LIBREALSENSE_DIR}/examples/third_party/glfw/include/ ${LIBREALSENSE_DIR}/include/)
set(LIBREALSENSE_LIB_PATH ${LIBREALSENSE_DIR}/ ${LIBREALSENSE_DIR}/bin/x64/ ${LIBREALSENSE_DIR}/examples/third_party/glfw/msvc140/obj/Release-x64)

#if your current path of opencv folder isn't C:/realsense/3rdparty/opencv, please update it
if(NOT DEFINED OPENCV_DIR)
set(OPENCV_DIR "C:/realsense/3rdparty/opencv")
endif(NOT DEFINED OPENCV_DIR)

set(OPENCV_INCLUDE_PATH ${OPENCV_DIR}/build/include/)
set(OPENCV_LIB_PATH ${OPENCV_DIR}/build/x64/vc14/lib ${OPENCV_DIR}/lib/Release)


#if your current path of log4cxx folder isn't C:/realsense/3rdparty/apache-log4cxx-0.10.0, please update it
if(NOT DEFINED LOG4CXX_DIR)
set(LOG4CXX_DIR "C:/realsense/3rdparty/apache-log4cxx-0.10.0")
endif(NOT DEFINED LOG4CXX_DIR)
set(LOG4CXX_INCLUDE_PATH ${LOG4CXX_DIR}/src/main/include/)


#if your current path of gtest folder isn't C:/realsense/3rdparty/gtest, please update it
if(NOT DEFINED GTEST_DIR)
set(GTEST_DIR "C:/realsense/3rdparty/gtest")
endif(NOT DEFINED GTEST_DIR)
set(GTEST_INCLUDE_PATH ${GTEST_DIR}/include/)
set(GTEST_LIB_PATH ${GTEST_DIR}/${CMAKE_BUILD_TYPE})

set(COMPILE_DEFINITIONS -Wall)
set(OPENCV_VER 310)
set(OPENGL_LIBS OpenGL32)
set(GLFW_LIBS glfw3)
set(SHLWAPI Shlwapi)
set(GTEST_LIBS gtest gtest_main-md)

include_directories(${OPENCV_INCLUDE_PATH} ${OPENCV_DIR} ${LIBREALSENSE_INCLUDE_PATH} ${LOG4CXX_INCLUDE_PATH} ${GTEST_INCLUDE_PATH})
link_directories(${LIBREALSENSE_LIB_PATH} ${OPENCV_LIB_PATH} ${GTEST_LIB_PATH})