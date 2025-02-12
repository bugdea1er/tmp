#ifndef TMP_CREATE_H
#define TMP_CREATE_H

#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {
namespace fs = std::filesystem;

/// Creates a temporary directory with the given label in the system's
/// temporary directory
/// @param[in] label A label to attach to the temporary directory path
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error  if cannot create a temporary directory
/// @throws std::invalid_argument if the label is ill-formatted
fs::path create_directory(std::string_view label);

/// Creates a temporary directory with the given label in the system's
/// temporary directory
/// @param[in]  label A label to attach to the temporary directory path
/// @param[out] ec    Parameter for error reporting
/// @returns A path to the created temporary directory
fs::path create_directory(std::string_view label, std::error_code& ec);

int create_file();

int create_file(std::error_code& ec);
}    // namespace tmp

#endif    // TMP_CREATE_H
