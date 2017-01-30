include(${SDK_DIR}/CMakeVersion)

set(SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/${SDK_DIR}/src/)
set(RELEASE_DIRS ${SRC_DIR}/cameras/playback/Release/ ${SRC_DIR}/cameras/record/Release/ ${SRC_DIR}/cameras/compression/Release/ ${SRC_DIR}/core/image/Release/
			${SRC_DIR}/core/pipeline/Release/ ${SRC_DIR}/core/projection/Release/ ${SRC_DIR}/cv_modules/max_depth_value_module/Release/ ${SRC_DIR}/utilities/command_line/Release/
			${SRC_DIR}/utilities/logger/log_utils/Release/ ${SRC_DIR}/utilities/samples_time_sync/Release/ ${SRC_DIR}/utilities/viewer/Release/ ${SRC_DIR}/core/image/lrs_image/Release/
)
set(EXE_DIRS ${SRC_DIR}/tools/capture_tool/Release/ ${SRC_DIR}/tools/projection_tool/Release/)

install(
DIRECTORY ${RELEASE_DIRS}
DESTINATION Libraries/
COMPONENT LIBS 
FILES_MATCHING PATTERN "*.dll" PATTERN "*.lib"
)

install(
DIRECTORY ${EXE_DIRS}
DESTINATION Tools/
COMPONENT TOOLS 
FILES_MATCHING PATTERN "*.exe"
)		
		
install(
DIRECTORY ${SDK_DIR}/include/ 
DESTINATION Include/
COMPONENT HEADERS 
FILES_MATCHING PATTERN "*.h"
)

install(
DIRECTORY ${LIBREALSENSE_DIR}/bin/x64/ 
DESTINATION 3rdparty/
COMPONENT LIBREALSENSE 
FILES_MATCHING PATTERN "*.dll"
)

install(
DIRECTORY ${OPENCV_DIR}/bin/Release/ 
DESTINATION 3rdparty/
COMPONENT OPENCV 
FILES_MATCHING PATTERN "*.dll"
)

install(
DIRECTORY ${GTEST_DIR}/Release/ 
DESTINATION 3rdparty/
COMPONENT GTEST 
FILES_MATCHING PATTERN "*.lib"
)

install(
DIRECTORY ${LZ4_DIR}/visual/VS2010/bin/x64_Release/
DESTINATION 3rdparty/
COMPONENT LZ4 
FILES_MATCHING PATTERN "*.dll"
)

install(
FILES "${CMAKE_SOURCE_DIR}/README.md"
DESTINATION Documentation/
COMPONENT DOCS
)

##########################################
# CPack configuration
##########################################

set(CPACK_PACKAGE_VERSION "${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "realsense_sdk.${CPACK_PACKAGE_VERSION}")
set(CPACK_PACKAGE_VENDOR "Intel")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RealSense SDK Installer")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Intel\\\\RealSense")

set(CPACK_COMPONENTS_ALL LIBS TOOLS HEADERS LIBREALSENSE OPENCV GTEST LZ4 DOCS)
set(CPACK_COMPONENT_LIBREALSENSE_GROUP "3rdparty")
set(CPACK_COMPONENT_OPENCV_GROUP "3rdparty")
set(CPACK_COMPONENT_GTEST_GROUP "3rdparty")
set(CPACK_COMPONENT_LZ4_GROUP "3rdparty")

set(CPACK_COMPONENT_LIBS_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_TOOLS_DISPLAY_NAME "Tools")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Headers")
set(CPACK_COMPONENT_LIBREALSENSE_DISPLAY_NAME "librealsense")
set(CPACK_COMPONENT_OPENCV_DISPLAY_NAME "OpenCV")
set(CPACK_COMPONENT_GTEST_DISPLAY_NAME "gtest")
set(CPACK_COMPONENT_LZ4_DISPLAY_NAME "lz4")
set(CPACK_COMPONENT_DOCS_DISPLAY_NAME "Documentation")

set(CPACK_COMPONENT_GROUP_3RDPARTY_DESCRIPTION "Install 3rd party dependencies")
set(CPACK_COMPONENT_LIBS_DESCRIPTION "Create dynamic and static libs")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "Include Headers")
set(CPACK_COMPONENT_TOOLS_DESCRIPTION "Create projection and capture tools")
set(CPACK_COMPONENT_DOCS_DESCRIPTION "Add documentation")


set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(CPACK_NSIS_PACKAGE_NAME "Intel RealSense SDK")
set(CPACK_NSIS_DISPLAY_NAME "RealSense SDK")

include(CPack)