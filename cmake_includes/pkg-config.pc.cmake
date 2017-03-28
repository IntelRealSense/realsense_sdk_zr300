prefix=${prefix}
exec_prefix=${exec_prefix}
libdir=${libdir}
includedir=${includedir}

Name: ${PACKAGE_NAME}
Description: ${CPACK_PACKAGE_DESCRIPTION_SUMMARY}
Version: ${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH}
Requires:
Libs: ${CPACK_PACKAGE_CONFIG_LIBS}
Cflags: ${CPACK_PACKAGE_CONFIG_CFLAGS}
