project(tmp-directory)

add_library(${PROJECT_NAME} INTERFACE)
add_library(tmp::directory ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_include_directories(${PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR})
#target_link_libraries(${PROJECT_NAME} INTERFACE std::filesystem)
