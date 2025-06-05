#include "abi.hpp"
#include "create.hpp"

#include <tmp/file>

#include <cstddef>
#include <filesystem>
#include <ios>
#include <istream>
#include <utility>

#ifdef __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

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
    : std::iostream(std::addressof(sb)),
      underlying(create_file(mode), &std::fclose) {
  mode |= std::ios::in | std::ios::out;

#if defined(_MSC_VER)
  sb = std::filebuf(underlying.get());
#elif defined(_LIBCPP_VERSION)
  sb.__open(fileno(underlying.get()), mode);
#else
  sb = __gnu_cxx::stdio_filebuf<char>(underlying.get(), mode);
#endif

  if (!sb.is_open()) {
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
  }
}

file::native_handle_type file::native_handle() const noexcept {
#ifdef _WIN32
  return reinterpret_cast<void*>(_get_osfhandle(_fileno(underlying.get())));
#else
  return fileno(underlying.get());
#endif
}

file::~file() noexcept = default;

// NOLINTBEGIN(*-use-after-move)
file::file(file&& other) noexcept
    : std::iostream(std::move(other)),
      underlying(std::move(other.underlying)),
      sb(std::move(other.sb)) {
  set_rdbuf(std::addressof(sb));
}
// NOLINTEND(*-use-after-move)

file& file::operator=(file&& other) = default;
}    // namespace tmp
