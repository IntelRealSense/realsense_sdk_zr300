cmake_minimum_required(VERSION 2.8.9)
project(realsense_pipeline)

include_directories(
    ${ROOT_DIR}/include/
)

set(SOURCE_FILES
    ${ROOT_DIR}/include/rs/core/pipeline_async_interface.h
    ${ROOT_DIR}/include/rs/core/pipeline_async.h
    sample_set_releaser.h
    pipeline_async_impl.h
    pipeline_async_impl.cpp
    pipeline_async.cpp
    samples_consumer_base.h
    samples_consumer_base.cpp
    sync_samples_consumer.h
    sync_samples_consumer.cpp
    async_samples_consumer.h
    async_samples_consumer.cpp
    streaming_device_manager.h
    streaming_device_manager.cpp
)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE}
    ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    realsense
    realsense_log_utils
    realsense_image
    realsense_lrs_image
    realsense_samples_time_sync
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBSOVERSION}")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
