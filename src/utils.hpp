#pragma once

#include <filesystem>

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

/// Creates a temporary path pattern with the given prefix and suffix
/// @param prefix   A prefix to be used in the path pattern
/// @param suffix   A suffix to be used in the path pattern
/// @returns A path pattern for the unique temporary path
/// @throws std::bad_alloc if memory allocation fails
fs::path make_pattern(std::string_view prefix, std::string_view suffix);
}    // namespace tmp
