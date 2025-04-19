#ifndef TMP_CREATE_H
#define TMP_CREATE_H

#include <filesystem>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <ios>
#endif

namespace tmp {
namespace fs = std::filesystem;

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory
/// @param[in] prefix A prefix to attach to the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error if cannot create a temporary directory
/// @throws std::invalid_argument if the prefix contains a directory separator
fs::path create_directory(std::string_view prefix);

#if defined(_WIN32)
/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in] mode The file opening mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
std::FILE* create_file(std::ios::openmode mode);

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in]  mode The file opening mode
/// @param[out] ec   Parameter for error reporting
/// @returns A handle to the created temporary file
std::FILE* create_file(std::ios::openmode mode, std::error_code& ec);
#elif __has_include(<unistd.h>)
/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
int create_file();

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[out] ec Parameter for error reporting
/// @returns A handle to the created temporary file
int create_file(std::error_code& ec);
#else
#error "Target platform not supported"
#endif
}    // namespace tmp

#endif    // TMP_CREATE_H
