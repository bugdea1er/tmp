#include <tmp/filebuf>

#include <ios>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#endif

#if __has_include(<__config>)
#include <__config>    // libc++ configuration
#endif

namespace tmp {

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<filebuf::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is as implemented in `entry`
static_assert(std::is_same_v<HANDLE, filebuf::native_handle_type>);
#endif

filebuf::filebuf() noexcept
    :
#if defined(_LIBCPP_VERSION) || defined(__GLIBCXX__)
      handle(-1)
#elif defined(_MSC_VER)
      handle(nullptr)
#elif
#error "Target C++ standard library is not supported"
#endif
{
}

filebuf* filebuf::open(open_handle_type handle, std::ios::openmode mode) {
  this->handle = handle;

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

  _Init(handle, _Openfl);
  return this;
#elif
#error "Target C++ standard library is not supported"
#endif
}

filebuf::native_handle_type filebuf::native_handle() const noexcept {
#ifdef _WIN32
  intptr_t osfhandle = _get_osfhandle(_fileno(handle));
  if (osfhandle == -1) {
    return nullptr;
  }

  return reinterpret_cast<void*>(osfhandle);
#else
  return handle;
#endif
}
}    // namespace tmp
