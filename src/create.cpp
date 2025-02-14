#include "create.hpp"

#include <tmp/filebuf>

#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

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

/// Checks that the given label is valid to attach to a temporary entry path
/// @param[in] label The label to check validity for
/// @returns `true` if the label is valid, `false` otherwise
bool is_label_valid(const fs::path& label) {
  return label.empty() || (++label.begin() == label.end() &&
                           label.is_relative() && !label.has_root_path() &&
                           label.filename() != "." && label.filename() != "..");
}

/// Checks that the given label is valid to attach to a temporary entry path
/// @param label The label to check validity for
/// @throws std::invalid_argument if the label cannot be attached to a path
void validate_label(const fs::path& label) {
  if (!is_label_valid(label)) {
    throw std::invalid_argument(
        "Cannot create a temporary entry: label must be empty or a valid "
        "single-segmented relative pathname");
  }
}

#ifdef _WIN32
/// Creates a temporary path with the given label
/// @note label must be valid
/// @param[in] label A label to attach to the path pattern
/// @returns A unique temporary path
fs::path make_path(std::string_view label) {
  constexpr static std::size_t CHARS_IN_GUID = 39;
  GUID guid;
  CoCreateGuid(&guid);

  wchar_t name[CHARS_IN_GUID];
  swprintf(name, CHARS_IN_GUID,
           L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
           guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
           guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
           guid.Data4[7]);

  return fs::temp_directory_path() / label / name;
}
#else
/// Creates a temporary path pattern with the given label
/// @note label must be valid
/// @param[in] label A label to attach to the path pattern
/// @returns A path pattern for the unique temporary path
fs::path make_pattern(std::string_view label) {
  return fs::temp_directory_path() / label / "XXXXXX";
}
#endif

/// Creates the parent directory of the given path if it does not exist
/// @param[in]  path The path for which the parent directory needs to be created
/// @param[out] ec   Parameter for error reporting
/// @returns `true` if a parent directory was newly created, `false` otherwise
bool create_parent(const fs::path& path, std::error_code& ec) {
  return fs::create_directories(path.parent_path(), ec);
}

#ifdef _WIN32
/// Makes a mode string for the `_wfdopen` function
/// @param mode The file opening mode
/// @returns A suitable mode string
const wchar_t* make_mdstring(std::ios::openmode mode) noexcept {
  switch (mode & ~std::ios::ate) {
  case std::ios::out:
  case std::ios::out | std::ios::trunc:
    return L"wx";
  case std::ios::out | std::ios::app:
  case std::ios::app:
    return L"a";
  case std::ios::in:
    return L"r";
  case std::ios::in | std::ios::out:
  case std::ios::in | std::ios::out | std::ios::trunc:
    return L"w+x";
  case std::ios::in | std::ios::out | std::ios::app:
  case std::ios::in | std::ios::app:
    return L"a+";
  case std::ios::out | std::ios::binary:
  case std::ios::out | std::ios::trunc | std::ios::binary:
    return L"wbx";
  case std::ios::out | std::ios::app | std::ios::binary:
  case std::ios::app | std::ios::binary:
    return L"ab";
  case std::ios::in | std::ios::binary:
    return L"rb";
  case std::ios::in | std::ios::out | std::ios::binary:
  case std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary:
    return L"w+bx";
  case std::ios::in | std::ios::out | std::ios::app | std::ios::binary:
  case std::ios::in | std::ios::app | std::ios::binary:
    return L"a+b";
  default:
    return nullptr;
  }
}
#endif

/// Closes the given handle, ignoring any errors
/// @param[in] handle The handle to close
void close(filebuf::native_handle_type handle) noexcept {
#ifdef _WIN32
  fclose(handle);
#else
  ::close(handle);
#endif
}
}    // namespace

std::pair<fs::path, filebuf> create_file(std::string_view label,
                                         std::ios::openmode mode) {
  validate_label(label);    // throws std::invalid_argument with a proper text

  std::error_code ec;
  auto file = create_file(label, mode, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}

std::pair<fs::path, filebuf> create_file(std::string_view label,
                                         std::ios::openmode mode,
                                         std::error_code& ec) {
  if (!is_label_valid(label)) {
    ec = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

#ifdef _WIN32
  fs::path::string_type path = make_path(label);
#else
  fs::path::string_type path = make_pattern(label);
#endif
  create_parent(path, ec);
  if (ec) {
    return {};
  }

  mode |= std::ios::in | std::ios::out;

#ifdef _WIN32
  std::FILE* handle;

  // FIXME: use _wfopen_s
  if (const wchar_t* mdstr = make_mdstring(mode)) {
    handle = _wfopen(path.c_str(), mdstr);
    if (handle == nullptr) {
      ec = std::error_code(errno, std::system_category());
      return {};
    }
  } else {
    ec = std::make_error_code(std::errc::invalid_argument);
    return {};
  }
#else
  int handle = mkstemp(path.data());
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return {};
  }
#endif

  filebuf filebuf;
  if (filebuf.open(handle, mode) == nullptr) {
    close(handle);
    ec = std::make_error_code(std::io_errc::stream);
    fs::remove(path);
    return {};
  }

  ec.clear();
  return std::make_pair(path, std::move(filebuf));
}

fs::path create_directory(std::string_view label) {
  validate_label(label);    // throws std::invalid_argument with a proper text

  std::error_code ec;
  fs::path directory = create_directory(label, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }

  return directory;
}

fs::path create_directory(std::string_view label, std::error_code& ec) {
  if (!is_label_valid(label)) {
    ec = std::make_error_code(std::errc::invalid_argument);
    return fs::path();
  }

#ifdef _WIN32
  fs::path::string_type path = make_path(label);
#else
  fs::path::string_type path = make_pattern(label);
#endif
  create_parent(path, ec);
  if (ec) {
    return fs::path();
  }

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
}    // namespace tmp
