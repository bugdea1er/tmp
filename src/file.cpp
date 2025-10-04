// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <tmp/file>

#include <cstdio>
#include <filesystem>
#include <system_error>
#include <type_traits>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#endif

namespace tmp::detail {

namespace fs = std::filesystem;

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<file::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is as implemented in `file`
static_assert(std::is_same_v<HANDLE, file::native_handle_type>);
#endif

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

/// Returns an implementation-defined handle to the file
/// @param[in] file The file to get the native handle for
/// @returns The underlying implementation-defined handle
file::native_handle_type get_native_handle(std::FILE* file) noexcept {
#ifdef _WIN32
  return reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
#else
  return fileno(file);
#endif
}
}    // namespace tmp::detail
