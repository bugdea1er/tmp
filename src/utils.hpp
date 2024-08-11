#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {

namespace fs = std::filesystem;

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;

/// Creates the parent directory of the given path if it does not exist
/// @param[in]  path The path for which the parent directory needs to be created
/// @param[out] ec   Parameter for error reporting
/// @returns @c true if a parent directory was newly created, @c false otherwise
/// @throws std::bad_alloc if memory allocation fails
bool create_parent(const fs::path& path, std::error_code& ec);

/// Creates a temporary path pattern with the given label and extension
/// @param label     A label to attach to the path pattern
/// @param extension An extension of the temporary file path
/// @returns A path pattern for the unique temporary path
/// @throws std::invalid_argument if the label or extension is ill-formatted
/// @throws std::bad_alloc        if memory allocation fails
fs::path make_pattern(std::string_view label, std::string_view extension);
}    // namespace tmp
