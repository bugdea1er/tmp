cmake_minimum_required(VERSION 3.12)
project(tmp LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        lib/directory.cpp
        lib/file.cpp
        lib/path.cpp)

add_library(tmp::tmp ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# GNU C++ Compiler requires linking with stdc++fs before version 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION LESS 9)
    target_link_libraries(${PROJECT_NAME} PUBLIC stdc++fs)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)
endif()
