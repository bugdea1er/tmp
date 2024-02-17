cmake_minimum_required(VERSION 3.12)
project(tmp LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
find_package(Filesystem REQUIRED)

add_library(${PROJECT_NAME} STATIC
        lib/directory.cpp
        lib/file.cpp
        lib/path.cpp)

add_library(tmp::tmp ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    endif()
endif()
