#include <tmp/filebuf>

#include <ios>

#ifdef _WIN32
#include <cstdio>
#include <corecrt_io.h>
#endif

#if __has_include(<__config>)
#include <__config>    // libc++ configuration
#endif

namespace tmp {
namespace {

#ifdef _WIN32
/// Makes a mode string for the `fdopen` function
/// @param mode The file opening mode
/// @returns A suitable mode string
const char* make_mdstring(std::ios::openmode mode) noexcept {
  switch (mode & ~std::ios::ate) {
  case std::ios::out:
  case std::ios::out | std::ios::trunc:
    return "w";
  case std::ios::out | std::ios::app:
  case std::ios::app:
    return "a";
  case std::ios::in:
    return "r";
  case std::ios::in | std::ios::out:
    return "r+";
  case std::ios::in | std::ios::out | std::ios::trunc:
    return "w+";
  case std::ios::in | std::ios::out | std::ios::app:
  case std::ios::in | std::ios::app:
    return "a+";
  case std::ios::out | std::ios::binary:
  case std::ios::out | std::ios::trunc | std::ios::binary:
    return "wb";
  case std::ios::out | std::ios::app | std::ios::binary:
  case std::ios::app | std::ios::binary:
    return "ab";
  case std::ios::in | std::ios::binary:
    return "rb";
  case std::ios::in | std::ios::out | std::ios::binary:
    return "r+b";
  case std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary:
    return "w+b";
  case std::ios::in | std::ios::out | std::ios::app | std::ios::binary:
  case std::ios::in | std::ios::app | std::ios::binary:
    return "a+b";
  default:
    return nullptr;
  }
}
#endif
}

filebuf* filebuf::open(native_handle_type handle, std::ios::openmode mode) {
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 70000
  // LLVM libc++ supports filesystem library since version 7
  // Apple LLVM, despite having a different version scheme still supplies original _LIBCPP_VERSION
  // Apple Clang supports filesystem library since version 11.0.0, and requires macOS 10.15 or newer (TODO: check other apple platforms?)
  return this->__open(handle, mode) ? this : nullptr;
#elif defined(__GLIBCXX__)    // TODO: check versions
  // GCC libstdc++ supports filesystem library since 8
  // https://en.cppreference.com/w/cpp/compiler_support/17#C.2B.2B17_library_features

  // On libstdc++ we replicate a standard `std::basic_filebuf::open`
  this->_M_file.sys_open(handle, mode);
  if (this->is_open()) {
    this->_M_allocate_internal_buffer();
    this->_M_mode = mode;

    // Setup initial buffer to 'uncommitted' mode
    this->_M_reading = false;
    this->_M_writing = false;
    this->_M_set_buffer(-1);

    // Reset to initial state
    this->_M_state_last = this->_M_state_cur = this->_M_state_beg;
    return this;
  }

  return nullptr;
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1914
  // MSVC STL supports filesystem library since version 19.14 (VS 2017 15.7)
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);
  std::FILE* file = _fdopen(fd, make_mdstring(mode));
  if (file == nullptr) {
    return nullptr;
  }

  _Init(file, _Newfl);
  return this;
#endif

  // Other C++ STL implementations may be supported if needed and possible
}
}
