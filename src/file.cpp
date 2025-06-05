#include "abi.hpp"
#include "create.hpp"

#include <tmp/file>

#include <cstddef>
#include <filesystem>
#include <ios>
#include <istream>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#include <corecrt_io.h>
#else
#include <unistd.h>
#endif

namespace tmp {

file::file(std::ios::openmode mode)
    : std::iostream(std::addressof(sb))
#if defined(_MSC_VER)
      ,
      underlying(create_file(mode), &std::fclose),
      sb(underlying.get())
#elif defined(_LIBCPP_VERSION)
      ,
      handle(create_file())
#endif
{
#ifndef _MSC_VER
  mode |= std::ios::in | std::ios::out;

#if defined(__GLIBCXX__)
  int handle = create_file();
  sb = __gnu_cxx::stdio_filebuf<char>(handle, mode);
#elif defined(_LIBCPP_VERSION)
  sb.__open(handle, mode);
#endif
#endif

  if (!sb.is_open()) {
#ifndef _WIN32
    close(handle);
#endif
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
  }
}

file::native_handle_type file::native_handle() const noexcept {
#if defined(__GLIBCXX__)
  return sb.fd();
#elif defined(_LIBCPP_VERSION)
  return handle;
#else    // MSVC
  return reinterpret_cast<void*>(_get_osfhandle(_fileno(underlying.get())));
#endif
}

file::~file() noexcept = default;

// NOLINTBEGIN(*-use-after-move)
file::file(file&& other) noexcept
    : std::iostream(std::move(other)),
#if defined(_MSC_VER)
      underlying(std::move(other.underlying)),
#elif defined(_LIBCPP_VERSION)
      handle(other.handle),
#endif
      sb(std::move(other.sb)) {
  set_rdbuf(std::addressof(sb));
}
// NOLINTEND(*-use-after-move)

file& file::operator=(file&& other) = default;
}    // namespace tmp
