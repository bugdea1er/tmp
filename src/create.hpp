#ifndef TMP_CREATE_H
#define TMP_CREATE_H

#include <tmp/filebuf>

#include <filesystem>
#include <ios>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {
namespace fs = std::filesystem;

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in] mode Specifies stream open mode
/// @returns A handle to the created temporary file
/// @throws fs::filesystem_error  if cannot create a temporary file
filebuf create_file(std::ios::openmode mode);

/// Creates a temporary file in the system's temporary directory,
/// and opens it for reading and writing
/// @param[in]  mode Specifies stream open mode
/// @param[out] ec   Parameter for error reporting
/// @returns A handle to the created temporary file
filebuf create_file(std::ios::openmode mode, std::error_code& ec);

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory
/// @param[in] prefix A prefix to attach to the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error  if cannot create a temporary directory
/// @throws std::invalid_argument if the prefix is ill-formatted
fs::path create_directory(std::string_view prefix);

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory
/// @param[in]  prefix A prefix to attach to the temporary directory name
/// @param[out] ec    Parameter for error reporting
/// @returns A path to the created temporary directory
fs::path create_directory(std::string_view prefix, std::error_code& ec);
}    // namespace tmp

#endif    // TMP_CREATE_H
