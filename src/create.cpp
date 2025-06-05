#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "create.hpp"

#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#define UNICODE
#include <Windows.h>
#include <array>
#include <corecrt_io.h>
#include <cwchar>
#else
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// Checks if the given prefix is valid to attach to a temporary directory name
/// @param[in] prefix The prefix to check validity for
/// @returns `true` if the prefix is valid, `false` otherwise
bool is_prefix_valid(const fs::path& prefix) {
  // We also need to check that the prefix does not contain a root path
  // because of how path concatenation works in C++
  return prefix.empty() || (++prefix.begin() == prefix.end() &&
                            prefix.is_relative() && !prefix.has_root_path());
}

#ifdef _WIN32
/// Creates a temporary path with the given prefix
/// @note prefix must be valid
/// @param[in] prefix A prefix to attach to the path pattern
/// @returns A unique temporary path
fs::path make_path(std::string_view prefix) {
  GUID guid;
  CoCreateGuid(&guid);

  constexpr static std::size_t CHARS_IN_GUID = 39;
  std::array name = std::array<wchar_t, CHARS_IN_GUID>();

  const wchar_t* format =
      prefix.empty() ? L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"
                     : L".%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";

  swprintf(name.data(), name.size(), format, guid.Data1, guid.Data2, guid.Data3,
           guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
           guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

  fs::path path = fs::temp_directory_path() / prefix;
  path += name.data();

  return path;
}

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
#endif
}    // namespace

fs::path create_directory(std::string_view prefix) {
  if (!is_prefix_valid(prefix)) {
    throw std::invalid_argument("Cannot create a temporary directory: "
                                "prefix cannot contain a directory separator");
  }

#ifdef _WIN32
  fs::path::string_type path = make_path(prefix);
#else
  std::string path = fs::temp_directory_path() / prefix;
  path += prefix.empty() ? "XXXXXX" : ".XXXXXX";
#endif

  std::error_code ec;
#ifdef _WIN32
  if (!CreateDirectory(path.c_str(), nullptr)) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  if (mkdtemp(path.data()) == nullptr) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }

  return path;
}

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
int create_file() {
  fs::path temp_directory_path = fs::temp_directory_path();
  int fd;

#ifdef O_TMPFILE
  // `O_TMPFILE` requires support by the underlying filesystem; if an unnamed
  // file cannot be created, we fall back to the generic method of creation
  fd = open(temp_directory_path.c_str(), O_RDWR | O_TMPFILE, S_IRUSR | S_IWUSR);
  if (fd != -1) {
    return fd;
  }
#endif

  std::string path = temp_directory_path / "XXXXXX";
  fd = mkstemp(path.data());
  if (fd == -1) {
    std::error_code ec = std::error_code(errno, std::system_category());
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  unlink(path.c_str());
  return fd;
}
#endif
}    // namespace tmp
