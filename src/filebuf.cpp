#include <tmp/filebuf>

#include "create.hpp"

#include <ios>

#if defined(_LIBCPP_VERSION)
#include <__config>    // libc++ configuration
#endif

namespace tmp {

filebuf* filebuf::open(std::ios::openmode mode) {
  handle = create_file();

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
#else
#error "Target C++ standard library is not supported"
#endif
}

filebuf::native_handle_type filebuf::native_handle() {
  return handle;
}
}    // namespace tmp
