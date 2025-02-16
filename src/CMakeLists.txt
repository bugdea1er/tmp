find_package(Filesystem REQUIRED)
include(GenerateExportHeader)

add_library(${PROJECT_NAME} create.cpp file.cpp directory.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)

set_target_properties(${PROJECT_NAME}
  PROPERTIES EXPORT_NAME ${PROJECT_NAME}
             CXX_EXTENSIONS OFF
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN ON
             VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR})

target_include_directories(${PROJECT_NAME}
  PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

if(TMP_IS_TOP_LEVEL)
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
  elseif(MSVC)
    # FIXME: warning C4251 for 'tmp::entry::pathobject'
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /wd4251)
  endif()
endif()

if(BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE TMP_BUILDING_DLL INTERFACE TMP_SHARED)
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
