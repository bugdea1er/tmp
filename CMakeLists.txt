option(TMP_MODULES "Build C++20 modules" OFF)
if(TMP_MODULES)
  cmake_minimum_required(VERSION 3.28)
else()
  cmake_minimum_required(VERSION 3.12)
endif()

project(tmp LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

add_library(${PROJECT_NAME} STATIC src/tmp.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)
add_library(tmp::tmp ALIAS ${PROJECT_NAME})

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  find_package(Filesystem REQUIRED)
  target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)
endif()

if(TMP_MODULES)
  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
  target_sources(${PROJECT_NAME}
    PUBLIC
      FILE_SET CXX_MODULES FILES
        src/tmp.cppm
)
else()
  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    elseif(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX /W3)
    endif()
endif()
