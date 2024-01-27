project(tmp-directory)

add_library(${PROJECT_NAME} INTERFACE)
add_library(tmp::directory ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_include_directories(${PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR})

# GNU C++ Compiler requires linking with stdc++fs before version 9
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION LESS 9)
    target_link_libraries(${PROJECT_NAME} INTERFACE stdc++fs)
endif()
