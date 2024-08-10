#include "utils.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#include <cwchar>
#endif

namespace tmp {

namespace {

/// Checks that the given label is valid to attach to a temporary entry path
/// @param label The label to check validity for
/// @throws std::invalid_argument if the label cannot be attached to a path
void validate_label(const fs::path& label) {
  if (label.empty()) {
    return;
  }

  if (++label.begin() != label.end() || label.is_absolute() ||
      label.has_root_path() || label.filename() == "." ||
      label.filename() == "..") {
    throw std::invalid_argument("");
  }
}

/// Checks that the given extension is valid to be an extension of a file path
/// @param extension The extension to check validity for
/// @throws std::invalid_argument if the extension cannot be used in a file path
void validate_extension(std::string_view extension) {
  if (extension.empty()) {
    return;
  }

  fs::path path = extension;
  if (++path.begin() != path.end()) {
    throw std::invalid_argument("");
  }
}
}    // namespace

bool create_parent(const fs::path& path, std::error_code& ec) {
  return fs::create_directories(path.parent_path(), ec);
}

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
}    // namespace tmp
