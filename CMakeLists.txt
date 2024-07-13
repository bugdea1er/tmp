cmake_minimum_required(VERSION 3.14)
project(tmp VERSION 0.9)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
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
            "${CMAKE_CURRENT_BINARY_DIR}/tmp-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/tmp-config-version.cmake"
          DESTINATION "${CMAKE_INSTALL_CMAKEDIR}")

  install(TARGETS ${PROJECT_NAME} EXPORT tmp-targets)
  install(EXPORT tmp-targets DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
    NAMESPACE tmp:: FILE tmp-targets.cmake)
  install(DIRECTORY "include/" "${PROJECT_BINARY_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

  include(CPack)
endif()
