project(${PROJECT_NAME}.test)

find_package(GTest)

add_executable(${PROJECT_NAME} directory.cpp file.cpp)
target_link_libraries(${PROJECT_NAME} tmp::tmp GTest::gtest_main)
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE PREFIX="com.github.bugdea1er.tmp")

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} EXTRA_ARGS --gtest_color=yes)
