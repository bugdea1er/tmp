#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <tmp/file>

#include <fcntl.h>
#include <filesystem>
#include <ios>
#include <istream>
#include <system_error>
#include <utility>

#ifdef __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#include <corecrt_io.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

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

/// Makes a mode string for opening a temporary file
/// @param[in] mode The file opening mode
/// @returns A suitable mode string
const char* make_mdstring(std::ios::openmode mode) noexcept {
  // Special case: the C++ standard forbids `app` and `trunc` at the same time
  if ((mode & std::ios::app) != 0 && (mode & std::ios::trunc) != 0) {
    return nullptr;
  }

  // - `std::ios::in` and `std::ios::out` are always applied
  // - `std::ios::trunc` has no effect on the empty file
  // - `std::ios::noreplace` has no effect for temporary files
  // - any other platform dependent flag is not supported
  unsigned filtered = mode & (std::ios::app | std::ios::binary);

  switch (filtered) {
  case 0:
#ifdef _WIN32
    return "w+TD";
#else
    return "w+";
#endif
  case std::ios::app:
#ifdef _WIN32
    return "a+TD";
#else
    return "a+";
#endif
  case std::ios::binary:
#ifdef _WIN32
    return "w+bTD";
#else
    return "w+b";
#endif
  case std::ios::app | std::ios::binary:
#ifdef _WIN32
    return "a+bTD";
#else
    return "a+b";
#endif
  default:
    return nullptr;
  }
}

/// Reopens the given temporary file with the given open mode
/// @param[in]  mdstring The temporary file opening mode
/// @param[out] file     The file to reopen
/// @param[out] ec       Parameter for error reporting
void reopen_file(const char* mdstring, std::FILE* file,
                 std::error_code& ec) noexcept {
  ec.clear();

#ifdef _WIN32
  HANDLE handle = get_native_handle(file);

  std::string path;
  path.resize(MAX_PATH);
  DWORD ret = GetFinalPathNameByHandleA(handle, path.data(), MAX_PATH, 0);
  if (ret == 0) {
    ec.assign(GetLastError(), std::system_category());
    return;
  }

  path.resize(ret);

  file = freopen(path.c_str(), mdstring, file);
  if (file == nullptr) {
    ec.assign(errno, std::generic_category());
  }
#else
  file = std::freopen(nullptr, mdstring, file);
  if (file == nullptr) {
    ec.assign(errno, std::generic_category());
  }
#endif
}

/// Creates and opens a temporary file in the current user's temporary directory
/// @param[in] mode The file opening mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
/// @throws std::invalid_argument if the given openmode is invalid
std::FILE* create_file(std::ios::openmode mode) {
  const char* mdstring = make_mdstring(mode);
  if (mdstring == nullptr) {
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
  }

  std::error_code ec;
  std::FILE* file = std::tmpfile();
  if (file == nullptr) {
    ec.assign(errno, std::generic_category());
  } else {
    reopen_file(mdstring, file, ec);
  }

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}
}    // namespace

file::file(std::ios::openmode mode)
    : std::iostream(std::addressof(sb)),
      underlying(create_file(mode), &std::fclose) {
  mode |= std::ios::in | std::ios::out;

#if defined(_MSC_VER)
  sb = std::filebuf(underlying.get());
#elif defined(_LIBCPP_VERSION)
  sb.__open(get_native_handle(underlying.get()), mode);
#else
  sb = __gnu_cxx::stdio_filebuf<char>(underlying.get(), mode);
#endif

  if (!sb.is_open()) {
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
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
