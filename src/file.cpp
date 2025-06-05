#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <tmp/file>

#include <fcntl.h>
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
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

#ifdef _WIN32
/// Makes a mode string for opening a temporary file using `_wfsopen`
/// @param[in] mode The file opening mode
/// @returns A suitable mode string
const wchar_t* make_mdstring(std::ios::openmode mode) noexcept {
  switch (mode & ~std::ios::in & ~std::ios::out & ~std::ios::ate) {
  case std::ios::openmode():
  case std::ios::trunc:
    return L"w+xTD";
  case std::ios::app:
    return L"a+TD";
  case std::ios::binary:
  case std::ios::trunc | std::ios::binary:
    return L"w+bxTD";
  case std::ios::app | std::ios::binary:
    return L"a+bTD";
  default:
    return nullptr;
  }
}

/// Creates and opens a temporary file in the current user's temporary directory
/// @param[in]  mode The file opening mode
/// @param[out] ec   Parameter for error reporting
/// @returns A handle to the created temporary file
/// @throws std::invalid_argument if the given openmode is invalid
std::FILE* create_file(std::ios::openmode mode, std::error_code& ec) {
  const wchar_t* mdstr = make_mdstring(mode);
  if (mdstr == nullptr) {
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
  }

  std::FILE* file = std::tmpfile();
  if (file == nullptr) {
    ec.assign(errno, std::generic_category());
    return nullptr;
  }

  HANDLE handle = reinterpret_cast<void*>(_get_osfhandle(_fileno(file)));

  std::wstring path;
  path.resize(MAX_PATH);
  DWORD ret = GetFinalPathNameByHandle(handle, path.data(), MAX_PATH, 0);
  if (ret == 0) {
    ec.assign(GetLastError(), std::system_category());
    return nullptr;
  }

  path.resize(ret);

  file = _wfreopen(path.c_str(), make_mdstring(mode), file);
  if (file == nullptr) {
    ec.assign(errno, std::generic_category());
    return nullptr;
  }

  ec.clear();
  return file;
}
#else
/// Makes a mode string for opening a file using `fopen`
/// @param[in] mode The file opening mode
/// @returns A suitable mode string
const char* make_mdstring(std::ios::openmode mode) noexcept {
  unsigned filtered = mode & ~std::ios::in & ~std::ios::out & ~std::ios::ate;
  switch (filtered) {
  case 0:
    return "r+";
  case std::ios::trunc:
    return "w+";
  case std::ios::app:
    return "a+";
  case std::ios::binary:
    return "r+b";
  case std::ios::trunc | std::ios::binary:
    return "w+b";
  case std::ios::app | std::ios::binary:
    return "a+b";
  default:
    return nullptr;
  }
}
#endif

/// Creates and opens a temporary file in the current user's temporary directory
/// @param[in] mode The file opening mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
/// @throws std::invalid_argument if the given openmode is invalid
std::FILE* create_file(std::ios::openmode mode);

#ifdef _WIN32
std::FILE* create_file(std::ios::openmode mode) {
  std::error_code ec;
  std::FILE* file = create_file(mode, ec);
  if (file == nullptr) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}
#else
std::FILE* create_file(std::ios::openmode mode) {
  const char* mdstr = make_mdstring(mode);
  if (mdstr == nullptr) {
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
  }

  std::FILE* file = std::tmpfile();
  if (file != nullptr) {
    file = freopen(nullptr, mdstr, file);
  }

  if (file == nullptr) {
    std::error_code ec = std::error_code(errno, std::system_category());
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}
#endif
}    // namespace

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
