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
}

std::filebuf* filebuf::open(native_handle_type handle, std::ios::openmode mode) {
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 70000
  // LLVM libc++ supports filesystem library since version 7
  // Apple LLVM, despite having a different version scheme still supplies original _LIBCPP_VERSION
  // Apple Clang supports filesystem library since version 11.0.0, and requires macOS 10.15 or newer (TODO: check other apple platforms?)
  return this->__open(handle, mode);
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
  if (handle == nullptr) {
    return nullptr;
  }

  _Init(handle, _Newfl);
  return this;
#endif

  // Other C++ STL implementations may be supported if needed and possible
}
}
