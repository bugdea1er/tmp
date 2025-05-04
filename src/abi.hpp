#ifndef TMP_ABI_H
#define TMP_ABI_H

#ifdef _WIN32
#define abi __declspec(dllexport)
#else
#define abi __attribute__((visibility("default")))
#endif

#include <filesystem>

namespace tmp {

class abi directory;

int create_file(void) abi;
void copy_file(const std::filesystem::path& from, int to) abi;
void close_file(int handle) noexcept abi;
}    // namespace tmp

#endif    // TMP_ABI_H
