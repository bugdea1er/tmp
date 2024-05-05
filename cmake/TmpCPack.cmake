set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/_packages")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ignat Sergeev")
set(CPACK_DEB_COMPONENT_INSTALL ON)

set(CPACK_DEBIAN_TMP_PACKAGE_NAME "libtmp")
set(CPACK_DEBIAN_TMP-DEV_PACKAGE_NAME "libtmp-dev")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_DEBIAN_TMP_PACKAGE_DEPENDS "libstdc++6 (>= 8)")
set(CPACK_DEBIAN_TMP-DEV_PACKAGE_DEPENDS
    "libstdc++-dev, libtmp (= ${CPACK_PACKAGE_VERSION})")

set(CPACK_DEBIAN_TMP_DESCRIPTION
    "RAII-wrappers for unique temporary files and directories that are deleted automatically for C++17 and later. Library package"
)
set(CPACK_DEBIAN_TMP-DEV_DESCRIPTION
    "RAII-wrappers for unique temporary files and directories that are deleted automatically for C++17 and later. Development package"
)

include(CPack)
