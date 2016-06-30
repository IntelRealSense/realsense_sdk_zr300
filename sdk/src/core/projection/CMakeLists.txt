cmake_minimum_required(VERSION 2.8.9)
project(rs_projection)

#------------------------------------------------------------------------------------

if(USE_IPP EQUAL 1)
    MESSAGE("---------    Using IPP------------")
    set(IPP_LIBS_LOCAL
        ${IPP_LIBS_TO_LINK}
    )
    add_definitions(-DUSE_IPP)
else()
    MESSAGE("---------    Using reference function for IPP ---------")
    set(IPP_REF_CODE
       rs_math.h
       math_lib/rsprojection.cpp
       math_lib/rsremap.cpp
       math_lib/rsuvmap.cpp
       math_lib/rsqrbacksubst_mva.cpp
       math_lib/rsqrdecomp_m.cpp
    )
endif()

#Source Files
set(SOURCE_FILES
    projection_r200.cpp
    projection_r200.h
    ${ROOT_DIR}/include/rs/core/projection.h
    ${IPP_REF_CODE}
)


#------------------------------------------------------------------------------------
#Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC") #-Wno-write-strings -g -O0

#------------------------------------------------------------------------------------
#Include
include_directories(${THIRDPARTY_DIR}/ipp/include)
include_directories(
    .
    ..
    ${ROOT_DIR}
    ${ROOT_DIR}/include
    ${ROOT_DIR}/include/rs/core
    ${ROOT_DIR}/src/utilities
    ${ROOT_DIR}/src/core/image
    ${ROOT_DIR}/src/utilities/image
)

#------------------------------------------------------------------------------------
#Building Library
add_library(${PROJECT_NAME} STATIC
    ${SOURCE_FILES}
)

#------------------------------------------------------------------------------------
#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
    ${IPP_LIBS_LOCAL}
    rs_log_utils
)


#------------------------------------------------------------------------------------
#Dependencies
add_dependencies(${PROJECT_NAME}
    rs_log_utils
)

#------------------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
