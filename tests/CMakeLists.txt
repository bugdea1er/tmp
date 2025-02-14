project(${PROJECT_NAME}.test)

find_package(GTest)

add_executable(${PROJECT_NAME} directory.cpp file.cpp)
target_link_libraries(${PROJECT_NAME} tmp::tmp GTest::gtest_main)
target_compile_definitions(${PROJECT_NAME}
  PRIVATE PREFIX="com.github.bugdea1er.tmp"
  BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}")

# On some platforms (e.g. Windows) CMake doesn't write load paths properly
# This solution to put outputs in the same directory is good enough
set_target_properties(${PROJECT_NAME}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:tmp::tmp>)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} EXTRA_ARGS --gtest_color=yes)
