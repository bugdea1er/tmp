project(${PROJECT_NAME}.test)

find_package(GTest)

add_executable(${PROJECT_NAME} directory.cpp file.cpp)
target_link_libraries(${PROJECT_NAME} tmp::tmp GTest::gtest_main)

# On some platforms (e.g. Windows) CMake doesn't write load paths properly
# This solution to put outputs in the same directory is good enough
set_target_properties(${PROJECT_NAME}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:tmp::tmp>)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} EXTRA_ARGS --gtest_color=yes)

find_package(benchmark REQUIRED)

add_executable(tmp-benchmark-writing benchmarkWriting.cpp)
target_link_libraries(tmp-benchmark-writing tmp::tmp benchmark::benchmark)

add_executable(tmp-benchmark-reading benchmarkReading.cpp)
target_link_libraries(tmp-benchmark-reading tmp::tmp benchmark::benchmark)
