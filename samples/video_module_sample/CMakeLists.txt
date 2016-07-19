cmake_minimum_required(VERSION 2.8)

project(rs_video_module_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SYNC_SAMPLE_PROJECT_NAME   rs_video_module_sync_sample)
set(ASYNC_SAMPLE_PROJECT_NAME  rs_video_module_async_sample)

set(SOURCES video_module_mock.h
            video_module_mock.cpp)

add_executable(${ASYNC_SAMPLE_PROJECT_NAME} video_module_async_sample.cpp ${SOURCES})
add_executable(${SYNC_SAMPLE_PROJECT_NAME} video_module_sync_sample.cpp ${SOURCES})

set(LIBRARIES_TO_LINK rs_image
                      rs_playback
                      rs_projection
                      rs_log_utils
                      realsense
                      pthread
                      opencv_imgproc opencv_core)

target_link_libraries(${ASYNC_SAMPLE_PROJECT_NAME} ${LIBRARIES_TO_LINK})
target_link_libraries(${SYNC_SAMPLE_PROJECT_NAME} ${LIBRARIES_TO_LINK})

install(TARGETS ${ASYNC_SAMPLE_PROJECT_NAME} DESTINATION bin)
install(TARGETS ${SYNC_SAMPLE_PROJECT_NAME} DESTINATION bin)
