cmake_minimum_required(VERSION 2.8)
project(rs_record_sample)

add_executable(${PROJECT_NAME} record_sample.cpp)

target_link_libraries(${PROJECT_NAME} rs_image realsense rs_record pthread)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
