// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#include "abi.hpp"

#include <filesystem>
#include <system_error>

namespace tmp::detail {

/// Deletes a path recursively, ignoring any errors
/// @param path The path to delete
void remove_all(const std::filesystem::path& path) noexcept {
  try {
    if (!path.empty()) {
      std::error_code ec;
      std::filesystem::remove_all(path, ec);
    }
  } catch (...) {
    // Do nothing: if we failed to delete the temporary directory,
    // the system should do it later
  }
}
}    // namespace tmp::detail
