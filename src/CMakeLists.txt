find_package(Filesystem REQUIRED)
include(GenerateExportHeader)

add_library(${PROJECT_NAME}-static STATIC tmp.cpp)
target_compile_features(${PROJECT_NAME}-static PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME}-static PUBLIC std::filesystem)

string(TOUPPER ${PROJECT_NAME} UPPER_PROJECT_NAME)
set_target_properties(${PROJECT_NAME}-static
  PROPERTIES EXPORT_NAME tmp
             CXX_EXTENSIONS OFF
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN 1
             COMPILE_FLAGS -D${UPPER_PROJECT_NAME}_STATIC_DEFINE
             OUTPUT_NAME tmp
             VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(${PROJECT_NAME}-static
  PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
         "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

generate_export_header(${PROJECT_NAME}-static
    BASE_NAME ${PROJECT_NAME}
    EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/include/tmp/export")

if(TMP_IS_TOP_LEVEL)
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
    target_compile_options(${PROJECT_NAME}-static PRIVATE -Werror)
    target_compile_options(${PROJECT_NAME}-static PRIVATE -Wall -Wextra -pedantic)
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME}-static PRIVATE /WX /W3)
  endif()
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}-static)

if(TMP_IS_TOP_LEVEL)
  add_library(${PROJECT_NAME}-shared SHARED tmp.cpp)
  target_compile_features(${PROJECT_NAME}-shared PUBLIC cxx_std_17)
  target_link_libraries(${PROJECT_NAME}-shared PUBLIC std::filesystem)

  set_target_properties(${PROJECT_NAME}-shared
    PROPERTIES EXPORT_NAME tmp
               CXX_EXTENSIONS OFF
               CXX_VISIBILITY_PRESET hidden
               VISIBILITY_INLINES_HIDDEN 1
               OUTPUT_NAME tmp
               VERSION ${PROJECT_VERSION}
               SOVERSION ${PROJECT_VERSION_MAJOR})

  target_include_directories(${PROJECT_NAME}-shared
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
           "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
    target_compile_options(${PROJECT_NAME}-shared PRIVATE -Werror)
    target_compile_options(${PROJECT_NAME}-shared PRIVATE -Wall -Wextra -pedantic)
  elseif(MSVC)
    target_compile_options(${PROJECT_NAME}-shared PRIVATE /WX /W3)
  endif()
endif()
