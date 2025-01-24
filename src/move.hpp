#ifndef TMP_MOVE_H
#define TMP_MOVE_H

#include <filesystem>
#include <system_error>

namespace tmp {
namespace fs = std::filesystem;

/// Moves the filesystem object as if by `std::filesystem::rename`
/// even when moving between filesystems
/// @param[in]  from The path to move
/// @param[in]  to   A path to the target file or directory
/// @param[out] ec   Parameter for error reporting
/// @throws std::bad_alloc if memory allocation fails
void move(const fs::path& from, const fs::path& to, std::error_code& ec);

/// Deletes the given path recursively, ignoring any errors
/// @param[in] path The path to delete
void remove(const fs::path& path) noexcept;
}    // namespace tmp

#endif    // TMP_MOVE_H
