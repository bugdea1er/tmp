cmake_minimum_required(VERSION 3.14)
project(tmp VERSION 0.9)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(TMP_IS_TOP_LEVEL OFF)
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(TMP_IS_TOP_LEVEL ON)
endif()

include(GNUInstallDirs)
add_subdirectory(src)

if(TMP_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tmp-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tmp")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/tmp-config-version.cmake"
        COMPATIBILITY AnyNewerVersion)
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/tmp-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/tmp-config-version.cmake"
              "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindFilesystem.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tmp")

    install(TARGETS ${PROJECT_NAME} EXPORT tmp-targets)
    install(
        EXPORT tmp-targets
        FILE tmp-targets.cmake
        NAMESPACE tmp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tmp")
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
                  "${CMAKE_CURRENT_BINARY_DIR}/include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    include(CPack)
endif()
