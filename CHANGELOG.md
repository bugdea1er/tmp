## v0.6.1 (May 01, 2024)

* Add option not to build tests

## v0.6 (May 01, 2024)

* Add install target ([#30](https://github.com/bugdea1er/tmp/pull/30))

## v0.5 (Mar 12, 2024)

* Add `tmp::file::copy` and `tmp::directory::copy` functions ([#20](https://github.com/bugdea1er/tmp/pull/20))

## v0.4 (Mar 03, 2024)

- Add `tmp::file::read` method ([#18](https://github.com/bugdea1er/tmp/pull/18))
- Add `tmp::file::slurp` method ([#19](https://github.com/bugdea1er/tmp/pull/19))

## v0.3 (Feb 18, 2024)

Added `FindFilesystem.cmake` module to configure the required filesystem library if it is not available by default ([#17](https://github.com/bugdea1er/tmp/pull/17))

When using this as a submodule, you can now use the new target `std::filesystem` to link it to your targets:
```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vendor/tmp/cmake/")
find_package(Filesystem REQUIRED)
...

add_executable(my-program main.cpp)
target_link_libraries(my-program PRIVATE std::filesystem)
```

## v0.2.2 (Feb 07, 2024)

Fixed an issue where moving between devices would behave differently than on the same device ([#16](https://github.com/bugdea1er/tmp/pull/16)):

- If moving between devices, fail if source is the file and target is a directory
- Remove the target recursively before copying over it

## v0.2.1 (Feb 04, 2024)

`tmp::path::move` now creates the parent directories of the given target if they do not exist ([#15](https://github.com/bugdea1er/tmp/pull/15))

## v0.2 (Feb 04, 2024)

- **Breaking change**: Removed file extensions for headers ([#13](https://github.com/bugdea1er/tmp/pull/13)). For example:
  ```cpp
  #include <tmp/directory>
  #include <tmp/path>
  ```
- Add `tmp::path::release` method ([#12](https://github.com/bugdea1er/tmp/pull/12))
- Add `tmp::path::move` method ([#14](https://github.com/bugdea1er/tmp/pull/14))

## v0.1 (Jan 30, 2024)

Initial Release
