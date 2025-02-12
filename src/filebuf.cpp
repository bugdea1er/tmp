#include <tmp/filebuf>

#include <ios>

#ifdef _WIN32
#include <corecrt_io.h>
#include <cstdio>
#endif

#if __has_include(<__config>)
#include <__config>    // libc++ configuration
#endif

namespace tmp {

filebuf* filebuf::open(native_handle_type handle, std::ios::openmode mode) {
#if defined(_LIBCPP_VERSION)
  return this->__open(handle, mode) != nullptr ? this : nullptr;
#elif defined(__GLIBCXX__)
  // We replicate the standard `std::basic_filebuf::open`
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
#elif defined(_MSC_VER)
  if (handle == nullptr) {
    return nullptr;
  }

  _Init(handle, _Newfl);
  return this;
#elif
#error "Target C++ standard library is not supported"
#endif
}
}    // namespace tmp
