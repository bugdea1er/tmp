#include "create.hpp"

#include <tmp/entry>

#include <filesystem>
#include <string_view>
#include <system_error>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <cwchar>
#endif

namespace tmp {
namespace {

/// Checks that the given label is valid to attach to a temporary entry path
/// @param[in] label The label to check validity for
/// @returns @c true if the label is valid, @c false otherwise
bool is_label_valid(const fs::path& label) {
  return label.empty() || (++label.begin() == label.end() &&
                              label.is_relative() && !label.has_root_path() &&
                              label.filename() != "." &&
                              label.filename() != "..");
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

/// Checks that the given extension is valid to be an extension of a file path
/// @param[in] extension The extension to check validity for
/// @returns @c true if the extension is valid, @c false otherwise
bool is_extension_valid(const fs::path& extension) {
  return extension.empty() || ++extension.begin() == extension.end();
}

/// Checks that the given extension is valid to be an extension of a file path
/// @param extension The extension to check validity for
/// @throws std::invalid_argument if the extension cannot be used in a file path
void validate_extension(std::string_view extension) {
  if (!is_extension_valid(extension)) {
    throw std::invalid_argument(
        "Cannot create a temporary file: extension must be empty or a valid "
        "single-segmented pathname");
  }
}

/// Creates a temporary path pattern with the given label and extension
/// @param label     A label to attach to the path pattern
/// @param extension An extension of the temporary file path
/// @returns A path pattern for the unique temporary path
/// @throws std::invalid_argument if the label or extension is ill-formatted
/// @throws std::bad_alloc        if memory allocation fails
fs::path make_pattern(std::string_view label, std::string_view extension) {
  validate_label(label);
  validate_extension(extension);

#ifdef _WIN32
  constexpr static std::size_t CHARS_IN_GUID = 39;
  GUID guid;
  CoCreateGuid(&guid);

  wchar_t name[CHARS_IN_GUID];
  swprintf(name, CHARS_IN_GUID,
           L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
           guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
           guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
           guid.Data4[7]);
#else
  std::string_view name = "XXXXXX";
#endif

  fs::path pattern = fs::temp_directory_path() / label / name;
  pattern += extension;

  return pattern;
}
}    // namespace

bool create_parent(const fs::path& path, std::error_code& ec) {
  return fs::create_directories(path.parent_path(), ec);
}

std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension) {
  fs::path::string_type path = make_pattern(label, extension);

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

#ifdef _WIN32
  HANDLE handle =
      CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  int handle = mkstemps(path.data(), static_cast<int>(extension.size()));
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create temporary file", ec);
  }

  return std::pair(path, handle);
}

std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension,
            std::error_code& ec);

std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label) {
  fs::path::string_type path = make_pattern(label, "");

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
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

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }

#ifdef _WIN32
  HANDLE handle =
      CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
#else
  int handle = open(path.data(), O_DIRECTORY);
#endif

  return std::pair(path, handle);
}

std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label, std::error_code& ec);
}    // namespace tmp
