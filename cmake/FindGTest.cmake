#[=======================================================================[.rst:
FindGTest
---------

Find the Google C++ Testing Framework

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``GTest::gtest``
  The Google Test ``gtest`` library, if found; adds Thread::Thread
  automatically
``GTest::gtest_main``
  The Google Test ``gtest_main`` library, if found
``GTest::gmock``
  The Google Mock ``gmock`` library, if found; adds Thread::Thread
  automatically
``GTest::gmock_main``
  The Google Mock ``gmock_main`` library, if found

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``GTest_FOUND``
  Found the Google Testing framework
``GTEST_INCLUDE_DIRS``
  the directory containing the Google Test headers
``GTEST_LIBRARIES``
  The Google Test ``gtest`` library; note it also requires linking
  with an appropriate thread library
``GTEST_MAIN_LIBRARIES``
  The Google Test ``gtest_main`` library
``GTEST_BOTH_LIBRARIES``
  Both ``gtest`` and ``gtest_main``

#]=======================================================================]

if(TARGET GTest::gtest)
    return()
endif()

find_package(GTest QUIET CONFIG)

if(GTest_FOUND)
    message(found)
    find_package_handle_standard_args(GTest CONFIG_MODE)
    return()
endif()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
set(INSTALL_GMOCK OFF)
set(INSTALL_GTEST OFF)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
