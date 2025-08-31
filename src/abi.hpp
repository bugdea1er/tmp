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

namespace tmp::detail {

auto abi create_directory(std::string_view prefix) -> std::filesystem::path;
auto abi remove_all(const std::filesystem::path& path) noexcept -> void;

auto abi create_file() -> std::FILE*;
auto abi get_native_handle(std::FILE* file) noexcept ->
#if defined(_WIN32)
    void*;
#elif __has_include(<unistd.h>)
    int;
#else
#error "Target platform not supported"
#endif
}    // namespace tmp::detail

#endif    // TMP_ABI_H
