## tmp

RAII-wrappers for unique temporary files and directories that are deleted automatically for C++17 and later.

This library provides functionality for efficient management of temporary files and directories in long-running applications, automatically deleting them as soon as your app is done with them to optimize resource management.

## Overview

When developing applications with long uptimes, such as server software, the management of temporary files and directories can become crucial. While the system may eventually clean up temporary files, this process may not occur frequently enough to meet the needs of applications with extended operational periods. Relying solely on the system for this task may lead to an accumulation of unnecessary temporary data on the device.

To address this challenge, this library provides functionality for creating temporary files and directories that are automatically deleted as soon as the application finishes using them. This ensures efficient resource management and prevents unnecessary clutter. Even in the event of a crash, the system will eventually clean up any temporary files created by this library, ensuring a seamless user experience and optimal performance.

The location of temporary files generated by our library will be determined by system variables such as:
- `TMPDIR`
- `TMP`
- `TEMP`
- `TEMPDIR`

If none of these variables are specified, the default path `/tmp` is used.

> [!NOTE]
> This library currently only works on Unix-like systems due to the use of `mkstemp` and `mkdtemp` functions, which may not be supported on other operating systems.
> 
> This library has been tested on:
> - macOS
> - Linux systems (e.g. Ubuntu Linux, Arch Linux, etc.)
> - FreeBSD (and should work on other BSD systems too)

## Examples

### tmp::directory
tmp::directory is a smart handle that owns and manages a temporary directory and deletes it recursively when this handle goes out of scope. This can be useful for setting up a directory to later use as a PWD for some process. For example,

```cpp
#include <tmp/directory>
...
auto tmpdir = tmp::directory("org.example.product");
fs::copy(file, tmpdir);

boost::process::child child {
  ...
  boost::process::start_dir = tmpdir->native(),
  ...
};

auto output = tmpdir / "output_file.txt";
```

### tmp::file
tmp::file, on the other hand, is a smart handle for a temporary file. It is also deleted if the handle goes out of scope. This can be useful, for example, for accepting a file from a socket and testing it before saving in a long-term storage:

```cpp
#include <tmp/file>
...
auto tmpfile = tmp::file("org.example.product");
tmpfile.write(data);

if (validate_metadata(tmpfile)) {
  tmpfile.move(storage / "new_file");
} else {
  throw InvalidRequestError();
}
```

## Integration

### CMake integration
You can also use the CMake interface target `tmp::tmp` and described below:

#### Embedded

To embed the library into your existing CMake project, place the entire source tree in a subdirectory (for example, using `git submodule` commands) and call `add_subdirectory()` in your `CMakeLists.txt` file:
```cmake
add_subdirectory(tmp)
...
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
