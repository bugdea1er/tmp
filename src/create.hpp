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

/// Implementation-defined handle type to the open file
#if defined(_WIN32)
using open_handle_type = std::FILE*;
#elif __has_include(<unistd.h>)
using open_handle_type = int;    // POSIX file descriptor
#else
#error "Target platform not supported"
#endif

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in] mode Specifies stream open mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error  if cannot create a temporary file
hidden open_handle_type create_file(std::ios::openmode mode);

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in]  mode Specifies stream open mode
/// @param[out] ec   Parameter for error reporting
/// @returns A handle to the created temporary file
hidden open_handle_type create_file(std::ios::openmode mode,
                                    std::error_code& ec);

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
/// @param[out] ec    Parameter for error reporting
/// @returns A path to the created temporary directory
hidden fs::path create_directory(std::string_view prefix, std::error_code& ec);
}    // namespace tmp

#endif    // TMP_CREATE_H
