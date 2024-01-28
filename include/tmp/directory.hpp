#pragma once

#include <tmp/path.hpp>

#include <filesystem>
#include <string_view>
#include <unistd.h>

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
    /// Creates a unique temporary directory using the system's default location
    /// for temporary files. If a prefix is provided to the constructor, the
    /// directory is created in the path <temp dir>/prefix/. The prefix can be
    /// a path consisting of multiple segments.
    explicit directory(std::string_view prefix = "") : path(prefix, creator) {}

    /// Concatenates this directory path with a given @p source
    std::filesystem::path operator/(std::string_view source) const {
        return this->underlying / source;
    }

    /// Deletes this directory recursively when the enclosing scope is exited
    ~directory() noexcept override = default;

    directory(directory&&) noexcept = default;               ///< move-constructible
    directory& operator=(directory&&) noexcept = default;    ///< move-assignable
    directory(const directory&) = delete;                    ///< not copy-constructible
    auto operator=(const directory&) = delete;               ///< not copy-assignable

private:
    /// Creates a unique temporary directory based on the given @p pattern path.
    /// The parent path of the given argument must exist.
    static std::string creator(std::string pattern) {
        if (mkdtemp(pattern.data()) == nullptr) {
            auto ec = std::error_code(errno, std::system_category());
            throw error("Cannot create temporary directory", ec);
        }

        return pattern;
    }
};

}    // namespace tmp
