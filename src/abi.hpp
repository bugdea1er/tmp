// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

#ifndef TMP_ABI_H
#define TMP_ABI_H

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

class abi file;
}    // namespace tmp

#endif    // TMP_ABI_H
