find_package(Filesystem REQUIRED)

add_library(${PROJECT_NAME} STATIC tmp.cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC std::filesystem)
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME tmp)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

if(TMP_USE_MODULES)
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
    target_sources(
        ${PROJECT_NAME}
        PUBLIC FILE_SET CXX_MODULES
        FILES tmp.cppm
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
    elseif(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX /W3)
    endif()
endif()
