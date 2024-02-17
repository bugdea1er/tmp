#[=======================================================================[.rst:
FindFilesystem
--------------

Find the C++17 standard library's filesystem utilities

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``std::filesystem``
  The filesystem library, if found.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Filesystem_FOUND``
  True if filesystem utilities are found.
#]=======================================================================]

if(Filesystem_FOUND AND TARGET std::filesystem)
    return()
endif()

include(CMakePushCheckState)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

cmake_push_check_state()

set(CMAKE_REQUIRED_QUIET TRUE)
set(CMAKE_REQUIRED_FLAGS -std=c++17)
set(STD_FILESYSTEM_CODE "
    #include <filesystem>
    #include <iostream>
    int main() {
        std::cout << std::filesystem::current_path() << std::endl;
        return 0;
    }
")

check_cxx_source_compiles("${STD_FILESYSTEM_CODE}" STD_FILESYSTEM_NO_LINK)
if(STD_FILESYSTEM_NO_LINK)
    set(STD_FILESYSTEM_FOUND TRUE)
else()
    set(CMAKE_REQUIRED_LIBRARIES -lstdc++fs)
    check_cxx_source_compiles("${STD_FILESYSTEM_CODE}" STD_FILESYSTEM_STDCXXFS)
    if(STD_FILESYSTEM_STDCXXFS)
        set(STD_FILESYSTEM_LIBRARY -lstdc++fs)
        set(STD_FILESYSTEM_FOUND TRUE)
    else()
        set(CMAKE_REQUIRED_LIBRARIES -lc++fs)
        check_cxx_source_compiles("${STD_FILESYSTEM_CODE}" STD_FILESYSTEM_CXXFS)
        if(STD_FILESYSTEM_STDCXXFS)
            set(STD_FILESYSTEM_LIBRARY -lc++fs)
            set(STD_FILESYSTEM_FOUND TRUE)
        endif()
    endif()
endif()

if(STD_FILESYSTEM_FOUND)
    set(Filesystem_FOUND TRUE CACHE BOOL "True if filesystem utilities are found")
    add_library(std::filesystem INTERFACE IMPORTED)
    if(STD_FILESYSTEM_LIBRARY)
        target_link_libraries(std::filesystem INTERFACE ${STD_FILESYSTEM_LIBRARY})
    endif()
else()
    if(Filesystem_FIND_REQUIRED)
        message(FATAL_ERROR "Cannot find filesystem utilities")
    endif()
endif()

cmake_pop_check_state()
