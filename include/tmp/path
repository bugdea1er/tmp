#pragma once

#include <filesystem>
#include <string_view>

namespace tmp {

/// tmp::path is a smart handle that owns and manages a temporary path and
/// deletes it recursively when this handle goes out of scope
///
/// The managed path is deleted of when either of the following happens:
/// - the managing tmp::path object is destroyed
/// - the managing tmp::path object is assigned another path via operator=
///
/// Subclass are encouraged to use tmp::path::make_pattern function to obtain
/// a temporary path pattern suitable for use in mktemp-like functions
class path {
    /// The managed path
    std::filesystem::path underlying;

public:
    /// Returns the managed path
    operator const std::filesystem::path&() const noexcept;

    /// Dereferences the managed path
    /// @returns A pointer to the path owned by *this
    const std::filesystem::path* operator->() const noexcept;

    /// Releases the ownership of the managed path;
    /// the destructor will not delete the managed path after the call
    /// @returns The managed path
    std::filesystem::path release() noexcept;

    /// Deletes the managed path recursively if it is not empty
    virtual ~path() noexcept;

    path(path&&) noexcept;                   ///< move-constructible
    path& operator=(path&&) noexcept;        ///< move-assignable
    path(const path&) = delete;              ///< not copy-constructible
    auto operator=(const path&) = delete;    ///< not copy-assignable

    /// Creates a path pattern with the given prefix
    ///
    /// The pattern consists of the system's temporary directory path, the given
    /// prefix, and six 'X' characters that must be replaced by random
    /// characters to ensure uniqueness
    ///
    /// The parent of the resulting path is created when this function is called
    /// @param prefix   A prefix to be used in the path pattern
    /// @returns A path pattern for the unique temporary path
    /// @throws std::filesystem::filesystem_error if failed to create the parent
    static std::filesystem::path make_pattern(std::string_view prefix);

protected:
    /// Constructs a tmp::path which owns @p path
    /// @param path A path to manage
    explicit path(std::filesystem::path path);
};

}    // namespace tmp
