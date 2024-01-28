project(tmp-directory)

add_library(${PROJECT_NAME} STATIC)
add_library(tmp::directory ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_sources(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/../lib/directory.cpp
        ${PROJECT_SOURCE_DIR}/../lib/file.cpp
        ${PROJECT_SOURCE_DIR}/../lib/path.cpp)

# GNU C++ Compiler requires linking with stdc++fs before version 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION LESS 9)
    target_link_libraries(${PROJECT_NAME} PUBLIC stdc++fs)
endif()
