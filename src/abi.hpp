#ifndef TMP_ABI_H
#define TMP_ABI_H

#include <cstdio>
#include <ios>

#ifdef _WIN32
#define abi __declspec(dllexport)
#else
#define abi __attribute__((visibility("default")))
#endif

namespace tmp {

class abi directory;

std::FILE* abi create_file(std::ios::openmode mode);

#ifdef _WIN32
void* abi get_native_handle(std::FILE* file) noexcept;
#else
int abi get_native_handle(std::FILE* file) noexcept;
#endif
}    // namespace tmp

#endif    // TMP_ABI_H
