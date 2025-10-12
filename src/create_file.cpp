// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <system_error>

namespace tmp::detail {

namespace fs = std::filesystem;

/// Creates and opens a binary temporary file as if by POSIX `tmpfile`
/// @returns A pointer to the file stream associated with the temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
std::FILE* create_file() {
  std::FILE* file = std::tmpfile();
  if (file == nullptr) {
    std::error_code ec = std::error_code(errno, std::generic_category());
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  return file;
}
}    // namespace tmp::detail
