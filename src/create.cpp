#include "create.hpp"

#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#define UNICODE
#include <Windows.h>
#include <cwchar>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// Checks if the given prefix is valid to attach to a temporary directory name
/// @param[in] prefix The prefix to check validity for
/// @returns `true` if the prefix is valid, `false` otherwise
bool is_prefix_valid(const fs::path& prefix) {
  return prefix.empty() ||
         (++prefix.begin() == prefix.end() && prefix.is_relative() &&
          !prefix.has_root_path() && prefix.filename() != "." &&
          prefix.filename() != "..");
}

/// Checks that the given prefix is valid to attach to a temporary entry path
/// @param prefix The prefix to check validity for
/// @throws std::invalid_argument if the prefix cannot be attached to a path
void validate_prefix(const fs::path& prefix) {
  if (!is_prefix_valid(prefix)) {
    throw std::invalid_argument(
        "Cannot create a temporary entry: prefix must be empty or a valid "
        "single-segmented relative pathname");
  }
}

#ifdef _WIN32
/// Creates a temporary path with the given prefix
/// @note prefix must be valid
/// @param[in] prefix A prefix to attach to the path pattern
/// @returns A unique temporary path
fs::path make_path(std::string_view prefix) {
  constexpr static std::size_t CHARS_IN_GUID = 39;
  GUID guid;
  CoCreateGuid(&guid);

  wchar_t name[CHARS_IN_GUID];
  swprintf(name, CHARS_IN_GUID,
           L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
           guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
           guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
           guid.Data4[7]);

  fs::path path = fs::temp_directory_path() / prefix;
  path += name;

  return path;
}
#else
/// Creates a temporary path pattern with the given prefix
/// @note prefix must be valid
/// @param[in] prefix A prefix to attach to the path pattern
/// @returns A path pattern for the unique temporary path
fs::path make_pattern(std::string_view prefix) {
  fs::path path = fs::temp_directory_path() / prefix;
  path += "XXXXXX";    // TODO: add '.', like `com.github.bugdea1er.tmp.yotR2k`?

  return path;
}
#endif

#ifdef _WIN32
/// Makes a mode string for the `_wfdopen` function
/// @param mode The file opening mode
/// @returns A suitable mode string
const wchar_t* make_mdstring(std::ios::openmode mode) noexcept {
  switch (mode & ~std::ios::ate) {
  case std::ios::out:
  case std::ios::out | std::ios::trunc:
    return L"wxD";
  case std::ios::out | std::ios::app:
  case std::ios::app:
    return L"aD";
  case std::ios::in:
    return L"rD";
  case std::ios::in | std::ios::out:
  case std::ios::in | std::ios::out | std::ios::trunc:
    return L"w+xD";
  case std::ios::in | std::ios::out | std::ios::app:
  case std::ios::in | std::ios::app:
    return L"a+D";
  case std::ios::out | std::ios::binary:
  case std::ios::out | std::ios::trunc | std::ios::binary:
    return L"wbxD";
  case std::ios::out | std::ios::app | std::ios::binary:
  case std::ios::app | std::ios::binary:
    return L"abD";
  case std::ios::in | std::ios::binary:
    return L"rbD";
  case std::ios::in | std::ios::out | std::ios::binary:
  case std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary:
    return L"w+bxD";
  case std::ios::in | std::ios::out | std::ios::app | std::ios::binary:
  case std::ios::in | std::ios::app | std::ios::binary:
    return L"a+bD";
  default:
    return nullptr;
  }
}
#endif
}    // namespace

fs::path create_directory(std::string_view prefix) {
  validate_prefix(prefix);    // throws std::invalid_argument with a proper text

  std::error_code ec;
  fs::path directory = create_directory(prefix, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }

  return directory;
}

fs::path create_directory(std::string_view prefix, std::error_code& ec) {
  if (!is_prefix_valid(prefix)) {
    ec = std::make_error_code(std::errc::invalid_argument);
    return fs::path();
  }

#ifdef _WIN32
  fs::path::string_type path = make_path(prefix);
#else
  fs::path::string_type path = make_pattern(prefix);
#endif

#ifdef _WIN32
  if (!CreateDirectory(path.c_str(), nullptr)) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  if (mkdtemp(path.data()) == nullptr) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  return path;
}

#if defined(_WIN32)
std::FILE* create_file(std::ios::openmode mode) {
  std::error_code ec;
  std::FILE* handle = create_file(mode, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return handle;
}

std::FILE* create_file(std::ios::openmode mode, std::error_code& ec) {
  fs::path::string_type path = make_path("");

  mode |= std::ios::in | std::ios::out;

  std::FILE* handle;
  if (const wchar_t* mdstr = make_mdstring(mode)) {
    handle = _wfsopen(path.c_str(), mdstr, _SH_DENYNO);
    if (handle == nullptr) {
      ec = std::error_code(errno, std::system_category());
      return nullptr;
    }
  } else {
    ec = std::make_error_code(std::errc::invalid_argument);
    return nullptr;
  }

  ec.clear();
  return handle;
}
#else
int create_file() {
  std::error_code ec;
  int handle = create_file(ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return handle;
}

int create_file(std::error_code& ec) {
  fs::path::string_type path = make_pattern("");

  int handle = mkstemp(path.data());
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return -1;
  }

  unlink(path.c_str());

  ec.clear();
  return handle;
}
#endif
}    // namespace tmp
