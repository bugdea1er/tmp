#include "create.hpp"

#include <tmp/entry>

#include <filesystem>
#include <random>
#include <string_view>
#include <system_error>
#include <unistd.h>

#ifndef _WIN32
#include <fcntl.h>
#else
#include <Windows.h>
#include <cwchar>
#endif

namespace tmp {
namespace {

/// Placeholder in temporary path templates to be replaced with random
/// characters
constexpr std::string_view placeholder = "XXXXXX";

/// Checks that the given label is valid to attach to a temporary entry path
/// @param[in] label The label to check validity for
/// @returns @c true if the label is valid, @c false otherwise
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
/// @note label and extension must be valid
/// @param[in] label     A label to attach to the path pattern
/// @param[in] extension An extension of the temporary file path
/// @returns A path pattern for the unique temporary path
fs::path make_pattern(std::string_view label, std::string_view extension) {
  fs::path pattern = fs::temp_directory_path() / label / placeholder;
  pattern += extension;

  return pattern;
}
}    // namespace

bool create_parent(const fs::path& path, std::error_code& ec) {
  return fs::create_directories(path.parent_path(), ec);
}

std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension) {
  validate_label(label);    // throws std::invalid_argument with a proper text
  validate_extension(extension);

  std::error_code ec;
  auto file = create_file(label, extension, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}

std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension,
            std::error_code& ec) {
  if (!is_label_valid(label) || !is_extension_valid(extension)) {
    ec = std::make_error_code(std::errc::invalid_argument);
    return std::pair<fs::path, entry::native_handle_type>();
  }

  fs::path::string_type path = make_pattern(label, extension);
  create_parent(path, ec);
  if (ec) {
    return std::pair<fs::path, entry::native_handle_type>();
  }

  int handle = mkstemps(path.data(), static_cast<int>(extension.size()));
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return std::pair<fs::path, entry::native_handle_type>();
  }

  ec.clear();
  return std::make_pair(path, handle);
}

std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label) {
  validate_label(label);    // throws std::invalid_argument with a proper text

  std::error_code ec;
  auto directory = create_directory(label, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }

  return directory;
}

std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label, std::error_code& ec) {
  if (!is_label_valid(label)) {
    ec = std::make_error_code(std::errc::invalid_argument);
    return std::pair<fs::path, entry::native_handle_type>();
  }

  fs::path::string_type path = make_pattern(label, "");
  create_parent(path, ec);
  if (ec) {
    return std::pair<fs::path, entry::native_handle_type>();
  }

  if (mkdtemp(path.data()) == nullptr) {
    ec = std::error_code(errno, std::system_category());
    return std::pair<fs::path, entry::native_handle_type>();
  }

  int handle = open(path.data(), O_SEARCH);
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return std::pair<fs::path, entry::native_handle_type>();
  }

  return std::pair(path, handle);
}
}    // namespace tmp
