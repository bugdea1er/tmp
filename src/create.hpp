#ifndef TMP_CREATE_H
#define TMP_CREATE_H

#include <filesystem>
#include <ios>
#include <string_view>
#include <system_error>

#if defined _WIN32
#define hidden
#else
#define hidden __attribute__((visibility("hidden")))
#endif

namespace tmp {
namespace fs = std::filesystem;

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory
/// @param[in] prefix A prefix to attach to the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error  if cannot create a temporary directory
/// @throws std::invalid_argument if the prefix is ill-formatted
hidden fs::path create_directory(std::string_view prefix);

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory
/// @param[in]  prefix A prefix to attach to the temporary directory name
/// @param[out] ec     Parameter for error reporting
/// @returns A path to the created temporary directory
hidden fs::path create_directory(std::string_view prefix, std::error_code& ec);

#if defined(_WIN32)
/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in] mode Specifies stream open mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
hidden std::FILE* create_file(std::ios::openmode mode);

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in]  mode Specifies stream open mode
/// @param[out] ec   Parameter for error reporting
/// @returns A handle to the created temporary file
hidden std::FILE* create_file(std::ios::openmode mode, std::error_code& ec);
#elif __has_include(<unistd.h>)
/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error if cannot create a temporary file
hidden int create_file();

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[out] ec Parameter for error reporting
/// @returns A handle to the created temporary file
hidden int create_file(std::error_code& ec);
#else
#error "Target platform not supported"
#endif
}    // namespace tmp

#endif    // TMP_CREATE_H
