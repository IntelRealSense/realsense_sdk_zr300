cmake_minimum_required(VERSION 2.8)
project(rs_tests)

FIND_PACKAGE(GTest REQUIRED)

include_directories(
    ${ROOT_DIR}
    ${ROOT_DIR}/include/rs/core
    ${ROOT_DIR}/src/cameras/include
    ${ROOT_DIR}/src/cameras/playback/include
    ${ROOT_DIR}/src/cameras/record/include
    ${ROOT_DIR}/src/core/image
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/utilities/logger/include
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
)

target_link_libraries(${PROJECT_NAME}
    gtest
    gtest_main
    pthread
    realsense
    rs_image
    rs_playback
    rs_record
    glfw
    rs_log_utils
    GL
)

add_dependencies(${PROJECT_NAME}
    rs_playback
    rs_record
    rs_log_utils
    rs_image
    rs_projection
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
