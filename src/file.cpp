#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <tmp/file>

#include <cstdio>
#include <filesystem>
#include <ios>
#include <system_error>
#include <type_traits>

#ifdef __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<file::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is as implemented in `file`
static_assert(std::is_same_v<HANDLE, file::native_handle_type>);
#endif

#ifndef _MSC_VER
/// Open mode for binary temporary files
constexpr auto mode = std::ios::binary | std::ios::in | std::ios::out;
#endif

/// Returns an implementation-defined handle to the file
/// @param[in] file The file to the native handle for
/// @returns The underlying implementation-defined handle
file::native_handle_type get_native_handle(std::FILE* file) noexcept {
#ifdef _WIN32
  return reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
#else
  return fileno(file);
#endif
}

/// Creates and opens a binary temporary file as if by POSIX `tmpfile`
/// @returns A pointer to the file stream associated with the temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
std::FILE* create_file() {
  std::FILE* file = std::tmpfile();
  if (file == nullptr) {
    std::error_code ec = std::error_code(errno, std::generic_category());
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}
}    // namespace

file::file()
    : std::iostream(std::addressof(sb)),
      underlying(create_file(), &std::fclose) {
#if defined(_MSC_VER)
  sb = std::filebuf(underlying.get());
#elif defined(_LIBCPP_VERSION)
  sb.__open(get_native_handle(underlying.get()), mode);
#else
  sb = __gnu_cxx::stdio_filebuf<char>(underlying.get(), mode);
#endif

  if (!sb.is_open()) {
    std::error_code ec = std::make_error_code(std::io_errc::stream);
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }
}

file::native_handle_type file::native_handle() const noexcept {
  return get_native_handle(underlying.get());
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
