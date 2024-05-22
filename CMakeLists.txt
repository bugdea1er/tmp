cmake_minimum_required(VERSION 3.12)

option(TMP_USE_MODULES "Build C++20 modules" OFF)
if(TMP_USE_MODULES)
  cmake_minimum_required(VERSION 3.28)
else()
endif()

project(tmp LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

include(GNUInstallDirs)
find_package(Filesystem REQUIRED)

add_library(${PROJECT_NAME} STATIC src/tmp.cpp)

if(TMP_USE_MODULES)
  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
  target_sources(${PROJECT_NAME}
    PUBLIC
      FILE_SET CXX_MODULES FILES
        src/tmp.cppm
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
)
else()
  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME tmp)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(tmp::tmp ALIAS ${PROJECT_NAME})

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    elseif(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX /W3)
    endif()

    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/tmp-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tmp)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/tmp-config.cmake
              ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindFilesystem.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tmp)

    install(
      TARGETS ${PROJECT_NAME}
      EXPORT tmp-targets
      FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
    install(
        EXPORT tmp-targets
        FILE TmpTargets.cmake
        NAMESPACE tmp::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tmp)
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
