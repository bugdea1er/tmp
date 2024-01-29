#pragma once

#include <tmp/path.hpp>

#include <filesystem>
#include <string_view>

namespace tmp {

/// tmp::directory is a smart handle that owns and manages a temporary directory
/// and disposes of it when this handle goes out of scope
///
/// When a tmp::directory object is created, it creates a unique temporary
/// directory using the system's default location for temporary files; the path
/// consists of the system's temporary directory path, the given prefix, and
/// random characters to ensure path uniqueness
///
/// The managed directory is deleted of when either of the following happens:
/// - the managing tmp::directory object is destroyed
/// - the managing tmp::directory object is assigned another path via operator=
///
/// The following example uses a tmp::directory object to create a temporary
/// directory with the product identifier prefix; when the function returns,
/// the tmp::directory object goes out of scope and the temporary directory is
/// deleted along with all of its contents:
///
/// @code{.cpp}
///   #include <tmp/directory.hpp>
///
///   auto func() {
///     auto tmpdir = tmp::directory("org.example.product);
///
///     // the temporary directory is deleted recursively when the
///     // tmp::directory object goes out of scope and is destroyed
///   }
/// @endcode
class directory final : public path {
public:
    /// Creates a unique temporary directory
    ///
    /// The directory path consists of the system's temporary directory path,
    /// the given prefix, and random characters to ensure path uniqueness
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
