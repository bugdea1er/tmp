#ifndef TMP_SYSTEM_H
#define TMP_SYSTEM_H

#include <tmp/entry>

#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {
namespace fs = std::filesystem;

/// Creates the parent directory of the given path if it does not exist
/// @param[in]  path The path for which the parent directory needs to be created
/// @param[out] ec   Parameter for error reporting
/// @returns @c true if a parent directory was newly created, @c false otherwise
bool create_parent(const fs::path& path, std::error_code& ec);

/// Creates a temporary file with the given label and extension in the system's
/// temporary directory, and opens it for reading and writing
/// @param[in] label     A label to attach to the temporary file path
/// @param[in] extension An extension of the temporary file path
/// @returns A path to the created temporary file and a handle to it
/// @throws fs::filesystem_error  if cannot create a temporary file
/// @throws std::invalid_argument if the label or extension is ill-formatted
std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension);

/// Creates a temporary file with the given label and extension in the system's
/// temporary directory, and opens it for reading and writing
/// @param[in]  label     A label to attach to the temporary file path
/// @param[in]  extension An extension of the temporary file path
/// @param[out] ec        Parameter for error reporting
/// @returns A path to the created temporary file and a handle to it
std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension,
            std::error_code& ec);

/// Creates a temporary directory with the given label in the system's
/// temporary directory, and opens it for searching
/// @param[in] label A label to attach to the temporary directory path
/// @returns A path to the created temporary file and a handle to it
/// @throws fs::filesystem_error  if cannot create a temporary directory
/// @throws std::invalid_argument if the label is ill-formatted
std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label);

/// Creates a temporary directory with the given label in the system's
/// temporary directory, and opens it for searching
/// @param[in]  label A label to attach to the temporary directory path
/// @param[out] ec    Parameter for error reporting
/// @returns A path to the created temporary directory and a handle to it
std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label, std::error_code& ec);
}    // namespace tmp

#endif    // TMP_SYSTEM_H
