cmake_minimum_required(VERSION 2.8)
include(ExternalProject)
project(rs_tests)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(SDK_DIR "${ROOT_DIR}/sdk")
include(${ROOT_DIR}/cmake_includes/check_os.cmake)

#Common settings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(${COMPILE_DEFINITIONS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 ")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)
endif()
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

#--------------Add security options --------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 ")  #TODO: Check what it is
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIE -fPIC")

#Add Gtest project
ExternalProject_Add(
    gtest_lib
    URL https://github.com/google/googletest/archive/release-1.8.0.zip
    URL_HASH SHA1=667f873ab7a4d246062565fad32fb6d8e203ee73
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
               -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
               -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
               -DBUILD_GMOCK=OFF -DBUILD_GTEST=ON
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)
# Set gtest properties
ExternalProject_Get_Property(gtest_lib source_dir binary_dir)
include_directories("${source_dir}/googletest/include")
link_directories("${binary_dir}/googletest/")

#build unit tests
include_directories(
    ${SDK_DIR}
    ${SDK_DIR}/include/rs/core
    ${SDK_DIR}/src/cameras/include
    ${SDK_DIR}/src/cameras/playback/include
    ${SDK_DIR}/src/cameras/record/include
    ${SDK_DIR}/src/core/image
    ${SDK_DIR}/src/utilities
    ${SDK_DIR}/src/utilities/logger/include
	${SDK_DIR}/src/include
	${SDK_DIR}/include
)

add_executable(${PROJECT_NAME}
    main.cpp
    projection_fixture.h
    utilities/utilities.h
    simple_streaming_tests.cpp
    playback_device_tests.cpp
    record_device_tests.cpp
    image_tests.cpp
    projection_tests.cpp
    librealsense_conversion_tests.cpp
    fps_counter_tests.cpp
    ${SDK_DIR}/include/rs/core/ref_count_interface.h
    ref_count_tests.cpp
    ${SAMPLES_TIME_SYNC_TESTS}
    ${PIPELINE_TEST}
	${FIND_DATA_PATH_TEST}
        rs_utils_tests.cpp)

target_link_libraries(${PROJECT_NAME}
    ${GTEST_LIBS}
    ${PTHREAD}
    ${GLFW_LIBS}
    realsense_max_depth_value_module
    realsense_pipeline
    realsense
    realsense_image
    realsense_playback
    realsense_record
    realsense_log_utils
    realsense_viewer
    realsense_projection
    realsense_samples_time_sync
)

add_dependencies(${PROJECT_NAME}
    realsense_max_depth_value_module
    realsense_pipeline
    realsense_image
    realsense_playback
    realsense_record
    realsense_log_utils
    realsense_viewer
    realsense_projection
    realsense_samples_time_sync
    gtest_lib
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
