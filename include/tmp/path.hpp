#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {

/// The tmp::path class is a smart handle that owns and manages a temporary path
/// and disposes of it when this handle goes out of scope.
///
/// Subclass this and provide mktemp-like function to the constructor to create
/// temporary files and directories.
class path {
public:
    /// Returns the underlying path
    operator const std::filesystem::path&() const noexcept;

    /// Provides access to the underlying path members
    const std::filesystem::path* operator->() const noexcept;

    /// Deletes this path recursively when the enclosing scope is exited
    virtual ~path() noexcept;

    path(path&&) noexcept;                   ///< move-constructible
    path& operator=(path&&) noexcept;        ///< move-assignable
    path(const path&) = delete;              ///< not copy-constructible
    auto operator=(const path&) = delete;    ///< not copy-assignable

protected:
    std::filesystem::path underlying;    ///< This file path

    /// Creates a unique temporary path using the given constructor function.
    /// @param prefix the path between system temp
    /// @param creator wrapped mktemp-like function that returns resulting path
    explicit path(std::filesystem::path path);

    /// Creates a pattern for the mktemp-like functions.
    /// If @p prefix is not empty, it is appended to the tempdir
    static std::string make_pattern(std::string_view prefix);
};

}    // namespace tmp
