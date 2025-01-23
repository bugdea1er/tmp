#include "create.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
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

/// Checks that the given extension is valid to be an extension of a file path
/// @param[in] extension The extension to check validity for
/// @returns `true` if the extension is valid, `false` otherwise
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

#ifdef _WIN32
/// Creates a temporary path with the given label and extension
/// @note label and extension must be valid
/// @param[in] label     A label to attach to the path pattern
/// @param[in] extension An extension of the temporary file path
/// @returns A unique temporary path
fs::path make_path(std::string_view label, std::string_view extension) {
  constexpr static std::size_t CHARS_IN_GUID = 39;
  GUID guid;
  CoCreateGuid(&guid);

  wchar_t name[CHARS_IN_GUID];
  swprintf(name, CHARS_IN_GUID,
           L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
           guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
           guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6],
           guid.Data4[7]);

  fs::path pattern = fs::temp_directory_path() / label / name;
  pattern += extension;

  return pattern;
}
#else
/// Placeholder in temporary path templates to be replaced
/// with random characters
constexpr std::string_view placeholder = "XXXXXX";

/// Creates a temporary path pattern with the given label and extension
/// @note label and extension must be valid
/// @param[in] label     A label to attach to the path pattern
/// @param[in] extension An extension of the temporary file path
/// @returns A path pattern for the unique temporary path
fs::path make_pattern(std::string_view label, std::string_view extension) {
  fs::path pattern = fs::temp_directory_path() / label / placeholder;
  pattern += extension;

  return pattern;
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

std::pair<fs::path, std::filebuf>
create_file(std::string_view label, std::string_view extension, bool binary) {
  validate_label(label);    // throws std::invalid_argument with a proper text
  validate_extension(extension);

  std::error_code ec;
  auto file = create_file(label, extension, binary, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}

std::pair<fs::path, std::filebuf> create_file(std::string_view label,
                                              std::string_view extension,
                                              bool binary,
                                              std::error_code& ec) {
  if (!is_label_valid(label) || !is_extension_valid(extension)) {
    ec = std::make_error_code(std::errc::invalid_argument);
    return {};
  }

#ifdef _WIN32
  fs::path::string_type path = make_path(label, extension);
#else
  fs::path::string_type path = make_pattern(label, extension);
#endif
  create_parent(path, ec);
  if (ec) {
    return {};
  }

#ifdef _WIN32
  HANDLE handle =
      CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    ec = std::error_code(GetLastError(), std::system_category());
    return {};
  }
#else
  // FIXME: `mkstemps` function is not a part of POSIX standard
  int handle = mkstemps(path.data(), static_cast<int>(extension.size()));
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return {};
  }
#endif

  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();
  mode |= std::ios::in | std::ios::out;

  std::filebuf filebuf;
  filebuf.open(path, mode);
  if (!filebuf.is_open()) {
    // TODO: better to ask the filebuf about `errc`
    ec = std::make_error_code(std::io_errc::stream);
    fs::remove(path);
  }

  ec.clear();
  // FIXME: opened handle for the file won't be closed
  return std::make_pair(path, std::move(filebuf));
}

fs::path create_directory(std::string_view label) {
  validate_label(label);    // throws std::invalid_argument with a proper text

  std::error_code ec;
  auto directory = create_directory(label, ec);

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
  fs::path::string_type path = make_path(label, "");
#else
  fs::path::string_type path = make_pattern(label, "");
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
