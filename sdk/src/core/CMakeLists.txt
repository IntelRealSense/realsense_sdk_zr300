cmake_minimum_required(VERSION 2.8)
project(core)

#------------------------------------------------------------------------------------
#Set
set(IPP_LIBS "${THIRDPARTY_DIR}/ipp/lib/intel64_linux")
set(IPP_LIBS_TO_LINK
    ${IPP_LIBS}/libipprs.a
    ${IPP_LIBS}/libippi.a
    ${IPP_LIBS}/libippe.a
    ${IPP_LIBS}/libipps.a
    ${IPP_LIBS}/libippdc.a
    ${IPP_LIBS}/libippcv.a
    ${IPP_LIBS}/libippcc.a
    ${IPP_LIBS}/libippch.a
    ${IPP_LIBS}/libippvm.a
    ${IPP_LIBS}/libippcore.a
)

#------------------------------------------------------------------------------------
#Subdirectories
add_subdirectory(image)
add_subdirectory(projection)

#------------------------------------------------------------------------------------
