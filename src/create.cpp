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
}    // namespace

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

int create_file() {
  std::error_code ec;
  int file = create_file(ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}

int create_file(std::error_code& ec) {
  fs::path::string_type path = make_pattern("");

  // FIXME: remove, since std::temp_directory_path requires p to exist
  create_parent(path, ec);
  if (ec) {
    return {};
  }

  // FIXME: add signal handling
  int handle = mkstemp(path.data());
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return -1;
  }

  unlink(path.data());

  ec.clear();
  return handle;
}
}    // namespace tmp
