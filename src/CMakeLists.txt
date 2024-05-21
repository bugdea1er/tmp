set(TMP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} STATIC)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME tmp)

target_sources(
    ${PROJECT_NAME}
    PRIVATE tmp.cpp
    PUBLIC FILE_SET HEADERS
           BASE_DIRS ${TMP_INCLUDE_DIR}/
           FILES ${TMP_INCLUDE_DIR}/tmp/file
                 ${TMP_INCLUDE_DIR}/tmp/directory
                 ${TMP_INCLUDE_DIR}/tmp/path
                 ${TMP_INCLUDE_DIR}/tmp/filesystem)

add_library(tmp::tmp ALIAS ${PROJECT_NAME})
