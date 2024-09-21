cmake_minimum_required(VERSION 3.16)
project(tmp
  VERSION 1.1
  DESCRIPTION "RAII-wrappers for unique temporary files and directories for modern C++"
  HOMEPAGE_URL "https://github.com/bugdea1er/tmp"
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
string(COMPARE EQUAL ${PROJECT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} TMP_IS_TOP_LEVEL)

include(GNUInstallDirs)
add_subdirectory(src)

if(TMP_IS_TOP_LEVEL)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()

  # Destination for cmake configuration files
  set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/")

  include(CMakePackageConfigHelpers)
  configure_package_config_file("cmake/Config.cmake.in" "tmp-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")
  write_basic_package_version_file("tmp-config-version.cmake"
    COMPATIBILITY AnyNewerVersion)
  install(FILES "cmake/modules/FindFilesystem.cmake"
            "${PROJECT_BINARY_DIR}/tmp-config.cmake"
            "${PROJECT_BINARY_DIR}/tmp-config-version.cmake"
          DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
          COMPONENT Development)

  install(TARGETS ${PROJECT_NAME} EXPORT tmp-targets
    COMPONENT Runtime)
  install(EXPORT tmp-targets DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
    NAMESPACE tmp:: FILE tmp-targets.cmake
    COMPONENT Development)
  install(DIRECTORY "include/" "${PROJECT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT Development)

  set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_SOURCE_DIR}/cmake/CPackConfig.cmake")
  include(CPack)
endif()
