find_package(Filesystem REQUIRED)
include(GenerateExportHeader)

add_library(${PROJECT_NAME} entry.cpp file.cpp directory.cpp utils.cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)

set_target_properties(${PROJECT_NAME}
  PROPERTIES EXPORT_NAME ${PROJECT_NAME}
             UNITY_BUILD ON
             CXX_EXTENSIONS OFF
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN ON
             VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(${PROJECT_NAME}
  PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
         "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

if(TMP_IS_TOP_LEVEL)
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
  elseif(MSVC)
    # FIXME: warning C4251 for 'tmp::entry::pathobject'
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /wd4251)
  endif()
endif()

generate_export_header(${PROJECT_NAME}
  EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}/export")

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
