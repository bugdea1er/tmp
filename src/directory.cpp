// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#include "abi.hpp"

#include <tmp/directory>

#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define UNICODE
#include <Windows.h>
#include <array>
#include <cwchar>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {
namespace {
namespace fs = std::filesystem;

/// Checks if the given prefix is valid to attach to a temporary directory name
/// @param prefix The prefix to check validity for
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
/// @param prefix A prefix to attach to the path pattern
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
#endif
}    // namespace

/// Creates a temporary directory in the current user's temporary directory
/// @param prefix A prefix to attach to the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error if cannot create a temporary directory
/// @throws std::invalid_argument if the prefix contains a directory separator
fs::path abi create_directory(std::string_view prefix) {
  if (!is_prefix_valid(prefix)) {
    throw std::invalid_argument("Cannot create a temporary directory: "
                                "prefix cannot contain a directory separator");
  }

  std::error_code ec;
#ifdef _WIN32
  fs::path path = make_path(prefix);
  if (!CreateDirectory(path.c_str(), nullptr)) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  std::string path = fs::temp_directory_path() / prefix;
  path += prefix.empty() ? "XXXXXX" : ".XXXXXX";

  if (mkdtemp(path.data()) == nullptr) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }

  return path;
}

/// Deletes a path recursively, ignoring any errors
/// @param path The path to delete
void abi remove_all(const fs::path& path) noexcept {
  try {
    if (!path.empty()) {
      // Calling the `std::error_code` overload of `fs::remove_all` should be
      // more optimal here since it would not require creating
      // a `fs::filesystem_error` message before we suppress the exception
      std::error_code ec;
      fs::remove_all(path, ec);
    }
  } catch (...) {
    // Do nothing: if we failed to delete the temporary directory,
    // the system should do it later
  }
}
}    // namespace tmp
