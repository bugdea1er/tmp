#pragma once

#include <tmp/path.hpp>

#include <filesystem>
#include <string_view>

namespace tmp {

/// The tmp::directory is a smart handle that owns and manages a temporary
/// directory and disposes of it when this handle goes out of scope. It
/// simplifies the process of creating and managing temporary directories by
/// ensuring that they are properly cleaned up when they are no longer needed.
///
/// When a tmp::directory object is created, it creates a unique temporary
/// directory using the system's default location for temporary files. If a
/// prefix is provided to the constructor, the directory is created in the path
/// <system's default location for temporary files>/prefix/. The prefix can be
/// a path consisting of multiple segments.
///
/// When the object is destroyed, it deletes the temporary recursively directory
///
/// @code{.cpp}
///   #include <tmp/directory.hpp>
///
///   auto func() {
///     tmp::directory tmpdir { "org.example.product" };
///
///     // use the temporary directory without worrying about cleanup
///
///     // the temporary directory is deleted recursively when the
///     // tmp::directory object goes out of scope and is destroyed
///   }
/// @endcode
///
/// The above example uses a tmp::directory object to create a temporary
/// directory with the product identifier prefix. When the function returns,
/// the tmp::directory object goes out of scope and the temporary directory is
/// deleted along with all of its contents.
class directory final : public path {
public:
    /// Creates a unique temporary directory
    ///
    /// The directory path consists of the system's temporary directory path,
    /// the given prefix, and six random characters to ensure path uniqueness
    ///
    /// @param prefix   A prefix to be used in the temporary directory path
    /// @throws std::filesystem::filesystem_error if cannot create a directory
    explicit directory(std::string_view prefix = "");

    /// Concatenates this directory path with a given @p source
    /// @param source   A string which represents a path name
    /// @returns The result of path concatenation
    std::filesystem::path operator/(std::string_view source) const;

    /// Deletes the managed directory recursively if its path is not empty
    ~directory() noexcept override;

    directory(directory&&) noexcept;               ///< move-constructible
    directory& operator=(directory&&) noexcept;    ///< move-assignable
    directory(const directory&) = delete;          ///< not copy-constructible
    auto operator=(const directory&) = delete;     ///< not copy-assignable
};

}    // namespace tmp
