// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#include "abi.hpp"

#include <tmp/file>

#include <cstdio>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#endif

namespace tmp::detail {

/// Returns an implementation-defined handle to the file
/// @param file The file to get the native handle for
/// @returns The underlying implementation-defined handle
file::native_handle_type get_native_handle(std::FILE* file) noexcept {
#ifdef _WIN32
  return reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
#else
  return fileno(file);
#endif
}
}    // namespace tmp::detail
