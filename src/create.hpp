#ifndef TMP_CREATE_H
#define TMP_CREATE_H

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {
namespace fs = std::filesystem;

/// Creates a temporary file with the given label and extension in the system's
/// temporary directory, and opens it for reading and writing
/// @param[in] label     A label to attach to the temporary file path
/// @param[in] extension An extension of the temporary file path
/// @param[in] mode      Specifies stream open mode
/// @returns A path to the created temporary file and a handle to it
/// @throws fs::filesystem_error  if cannot create a temporary file
/// @throws std::invalid_argument if the label or extension is ill-formatted
std::pair<fs::path, std::FILE*> create_file(std::string_view label,
                                            std::string_view extension,
                                            std::ios::openmode mode);

/// Creates a temporary file with the given label and extension in the system's
/// temporary directory, and opens it for reading and writing
/// @param[in]  label     A label to attach to the temporary file path
/// @param[in]  extension An extension of the temporary file path
/// @param[in]  mode      Specifies stream open mode
/// @param[out] ec        Parameter for error reporting
/// @returns A path to the created temporary file and a handle to it
std::pair<fs::path, std::FILE*> create_file(std::string_view label,
                                            std::string_view extension,
                                            std::ios::openmode mode,
                                            std::error_code& ec);

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
}    // namespace tmp

#endif    // TMP_CREATE_H
