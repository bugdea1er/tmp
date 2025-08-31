// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#ifndef TMP_ABI_H
#define TMP_ABI_H

#include <cstdio>
#include <filesystem>
#include <string_view>

#ifdef _WIN32
#define abi __declspec(dllexport)
#else
#define abi __attribute__((visibility("default")))
#endif

namespace tmp {

std::filesystem::path abi create_directory(std::string_view prefix);
void abi remove_all(const std::filesystem::path& path) noexcept;

std::FILE* abi create_file();

#if defined(_WIN32)
void* abi get_native_handle(std::FILE* file) noexcept;
#elif __has_include(<unistd.h>)
int abi get_native_handle(std::FILE* file) noexcept;
#endif
}    // namespace tmp

#endif    // TMP_ABI_H
