#ifndef TMP_CREATE_H
#define TMP_CREATE_H

#include <filesystem>
#include <string_view>

#ifdef _WIN32
#include <ios>
#endif

namespace tmp {
namespace fs = std::filesystem;

/// Creates a temporary directory in the current user's temporary directory
/// @param[in] prefix A prefix to attach to the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error if cannot create a temporary directory
/// @throws std::invalid_argument if the prefix contains a directory separator
fs::path create_directory(std::string_view prefix);

#ifdef _WIN32
/// Creates and opens a temporary file in the current user's temporary directory
/// @param[in] mode The file opening mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
/// @throws std::invalid_argument if the given openmode is invalid
std::FILE* create_file(std::ios::openmode mode);
#else
/// Creates and opens a temporary file in the current user's temporary directory
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
int create_file();
#endif
}    // namespace tmp

#endif    // TMP_CREATE_H
