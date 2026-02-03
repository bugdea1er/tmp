// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <cerrno>
#include <cstdio>
#include <system_error>

namespace tmp::detail {

/// Creates and opens a binary temporary file as if by POSIX `tmpfile`
/// @param ec Set on failure
/// @returns A pointer to the file stream, or nullptr on failure
std::FILE* create_file(std::error_code& ec) noexcept {
  ec.clear();
  std::FILE* file = std::tmpfile();
  if (file == nullptr) {
    ec = std::error_code(errno, std::generic_category());
  }
  return file;
}
}    // namespace tmp::detail
