project(${PROJECT_NAME}.test)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TESTS file_test.cpp directory_test.cpp)
source_group("Tests" FILES ${TESTS})

add_executable(${PROJECT_NAME} ${TESTS})
target_link_libraries(${PROJECT_NAME} tmp::tmp GTest::gtest_main)
target_compile_definitions(${PROJECT_NAME} PRIVATE PREFIX="com.github.bugdea1er.tmp")

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
