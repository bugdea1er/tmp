#[=======================================================================[.rst:
FindFilesystem
--------------

Find the C++17 standard library's filesystem utilities

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``std::filesystem``
  The filesystem library, if found

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``Filesystem_FOUND``
  Found the filesystem utilities
``Filesystem_LIBRARIES``
  The filesystem utilities library

#]=======================================================================]

if(Filesystem_FOUND AND TARGET std::filesystem)
  return()
endif()

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)
include(FindPackageHandleStandardArgs)

cmake_push_check_state()
set(CMAKE_REQUIRED_QUIET TRUE)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU|Clang)
  set(CMAKE_REQUIRED_FLAGS -std=c++17)
elseif(MSVC)
  set(CMAKE_REQUIRED_FLAGS -std:c++17)
endif()

set(Filesystem_TEST_SOURCE
  "
  #include <filesystem>
  #include <iostream>
  int main() {
    std::cout << std::filesystem::current_path() << std::endl;
    return 0;
  }
")

check_cxx_source_compiles("${Filesystem_TEST_SOURCE}" STD_FILESYSTEM_NO_LINK)
if(STD_FILESYSTEM_NO_LINK)
  set(STD_FILESYSTEM_FOUND TRUE)
else()
  set(CMAKE_REQUIRED_LIBRARIES -lstdc++fs)
  check_cxx_source_compiles("${Filesystem_TEST_SOURCE}" STD_FILESYSTEM_STDCXXFS)
  if(STD_FILESYSTEM_STDCXXFS)
    set(STD_FILESYSTEM_FOUND TRUE)
    set(STD_FILESYSTEM_LIBRARY stdc++fs)
  else()
    set(CMAKE_REQUIRED_LIBRARIES -lc++fs)
    check_cxx_source_compiles("${Filesystem_TEST_SOURCE}" STD_FILESYSTEM_CXXFS)
    if(STD_FILESYSTEM_STDCXXFS)
      set(STD_FILESYSTEM_FOUND TRUE)
      set(STD_FILESYSTEM_LIBRARY c++fs)
    endif()
  endif()
endif()

cmake_pop_check_state()

set(Filesystem_FOUND ${STD_FILESYSTEM_FOUND}
  CACHE BOOL "True if filesystem utilities are found")
set(Filesystem_LIBRARIES ${STD_FILESYSTEM_LIBRARY}
  CACHE STRING "The filesystem utilities libraries")
mark_as_advanced(STD_FILESYSTEM_FOUND STD_FILESYSTEM_LIBRARY)

if(Filesystem_LIBRARIES)
  find_package_handle_standard_args(Filesystem DEFAULT_MSG Filesystem_LIBRARIES)
else()
  find_package_handle_standard_args(Filesystem DEFAULT_MSG Filesystem_FOUND)
endif()

if(Filesystem_FOUND AND NOT TARGET std::filesystem)
  add_library(std::filesystem INTERFACE IMPORTED)
  target_link_libraries(std::filesystem INTERFACE ${Filesystem_LIBRARIES})
endif()
