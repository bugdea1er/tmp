// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS    // NOLINT

#include "abi.hpp"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

#if __has_include(<unistd.h>)
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tmp::detail {

namespace fs = std::filesystem;

namespace {

#if __has_include(<unistd.h>)
/// Creates and opens a binary temporary file as if by POSIX `mkstemp`
/// @returns A file descriptor associated with the temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
int create_file_descriptor() {
  fs::path temp_dir = fs::temp_directory_path();
  int descriptor    = int{};

#ifdef __linux__
  descriptor = open(temp_dir.c_str(), O_RDWR | O_TMPFILE, S_IRUSR | S_IWUSR);
  if (descriptor >= 0) {
    return descriptor;
  }
#endif

  std::string path = temp_dir / "XXXXXX";

  descriptor = mkstemp(path.data());
  if (descriptor == -1) {
    std::error_code ec = std::error_code(errno, std::system_category());
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

  unlink(path.data());

  return descriptor;
}
#endif
}    // namespace

/// Creates and opens a binary temporary file as if by POSIX `tmpfile`
/// @returns A pointer to the file stream associated with the temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
std::FILE* create_file() {
#if __has_include(<unistd.h>)
  int descriptor = create_file_descriptor();

  // TODO: let `filebuf` use the file descriptor without `FILE` wrapping
  std::FILE* file = fdopen(descriptor, "wb+");
  if (file == nullptr) {
    std::error_code ec = std::error_code(errno, std::system_category());
    close(descriptor);

    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }
#else
  std::FILE* file = std::tmpfile();
  if (file == nullptr) {
    std::error_code ec = std::error_code(errno, std::generic_category());
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }
#endif

  return file;
}
}    // namespace tmp::detail
