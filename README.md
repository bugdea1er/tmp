## tmp

RAII-wrappers for unique temporary files and directories that are deleted automatically for C++17 and later.

Currently only works on UNIX-like systems because it uses functions similar to mktemp. Tested on linux and macOS systems.

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

## Current plans
- Provide install targets https://github.com/bugdea1er/tmp/issues/11
- Make this work on Windows https://github.com/bugdea1er/tmp/issues/3

## Contributing
Contributions are always welcome!
