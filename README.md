## tmp

RAII-wrappers for unique temporary files and directories that are deleted automatically for C++17 and later.

This library provides functionality for efficient management of temporary files and directories in long-running applications, automatically deleting them as soon as your app is done with them to optimize resource management.

![Static Badge](https://img.shields.io/badge/C%2B%2B-17%2B-blue)
![GitHub Tag](https://img.shields.io/github/v/release/bugdea1er/tmp)


## Overview

When developing applications with long uptimes, such as server software, the management of temporary files and directories can become crucial. While the system may eventually clean up temporary files, this process may not occur frequently enough to meet the needs of applications with extended operational periods. Relying solely on the system for this task may lead to an accumulation of unnecessary temporary data on the device.

To address this challenge, this library provides functionality for creating temporary files and directories that are automatically deleted as soon as the application finishes using them. This ensures efficient resource management and prevents unnecessary clutter. Even in the event of a crash, the system will eventually clean up any temporary files created by this library, ensuring a seamless user experience and optimal performance.

 This library has been tested on:
 - macOS
 - Linux systems (e.g. Ubuntu Linux, Arch Linux, etc.)
 - FreeBSD (and should work on other BSD systems too)
 - Windows


## Features

### Temporary directories

`tmp::directory` is a smart handle that manages a temporary directory, ensuring its recursive deletion when the handle goes out of scope. When a `tmp::directory` object is created, it generates a unique temporary directory in the current user's temporary directory.

The temporary directory is deleted when either of the following happens: 
- the `tmp::directory` object is destroyed
- the `tmp::directory` object is assigned another directory using `operator=`

The example below demonstrates a usage of a `tmp::directory` object to run a subprocess within it and archive its logs; when the function returns, the temporary directory is recursively deleted:

```cpp
#include <tmp/directory>

auto func() {
  auto tmpdir = tmp::directory("org.example.product");
  process::exec(executable, args, tmpdir);

  return archive::glob(tmpdir, "*.log");

  // The temporary directory is deleted recursively when the
  // tmp::directory object goes out of scope
}
```

### Temporary files

`tmp::file` is a smart handle that manages a binary temporary file, ensuring its deletion when the handle goes out of scope. Upon creation, a `tmp::file` object generates a unique temporary file, opening it for both reading and writing in binary format.

The temporary file is deleted of when either of the following happens:
- the `tmp::file` object is destroyed
- the `tmp::file` object is assigned another file using `operator=`

`tmp::file` inherits from the `std::iostream` class, allowing it to be used seamlessly with standard input/output operations and simplifying file handling while maintaining the flexibility of stream operations.

The example below demonstrates a usage of a `tmp::file` object to validate a request content and then unarchive it to persistent storage:

```cpp
#include <tmp/file>

auto func(std::string_view content) {
  auto tmpfile = tmp::file();
  tmpfile << contents << std::flush;
  if (validate(tmpfile)) {
    // Unarchive the file to the persistent storage
    archive::unzip(tmpfile, storage);
  } else {
    // The file is deleted automatically
    throw InvalidRequestError();
  }
}
```

## Building

To build the project without tests, use the `-DBUILD_TESTING=OFF` flag:
```shell
cmake -B build -DBUILD_TESTING=OFF && cmake --build build
```

To build the project with tests, run:
```shell
cmake -B build && cmake --build build --target tmp.test
```

To run tests:
```shell
ctest --output-on-failure --test-dir build
```

## Installing

To install the project, configure it in the Release configuration:
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON
```

build it:
```shell
cmake --build build
```

and then install:
```shell
sudo cmake --install build
```

OR use `dpkg` package manager (requires `dpkg-shlibdeps`):
```shell
cd build && cpack -G DEB && sudo dpkg -i *.deb
```

OR use `rpm` package manager (requires `rpmbuild`):
```shell
cd build && cpack -G RPM && sudo rpm -i *.rpm
```

## Package managers

If you are using homebrew, use the following command:
```shell
brew install bugdea1er/tap/tmp
```

## Integration

### CMake integration
You can also use the CMake interface target `tmp::tmp` and described below:

### External

To use this library from a CMake project, locate it using `find_package`:
```cmake
find_package(tmp REQUIRED)
# ...
add_library(foo ...)
target_link_libraries(foo PUBLIC tmp::tmp)
```

#### Embedded

To embed the library into your existing CMake project, place the entire source tree in a subdirectory (for example, using `git submodule` commands) and call `add_subdirectory()` in your `CMakeLists.txt` file:
```cmake
add_subdirectory(tmp)
# ...
add_library(foo ...)
target_link_libraries(foo PUBLIC tmp::tmp)
```

#### FetchContent

You can also use the FetchContent functions to automatically download a dependency. Put this in your `CMakeLists.txt` file:
```cmake
include(FetchContent)

FetchContent_Declare(tmp URL https://github.com/bugdea1er/tmp/releases/download/<version>/tmp.tar.xz)
FetchContent_MakeAvailable(tmp)

target_link_libraries(foo PUBLIC tmp::tmp)
```

## Contributing
Contributions are always welcome!
