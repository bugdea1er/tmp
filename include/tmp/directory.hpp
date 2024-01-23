#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>
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
class directory {
    std::filesystem::path p;    ///< This directory path

    /// Deletes this directory recursively
    void remove() const noexcept {
        if (!this->p.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(this->p, ec);
        }
    }

public:
    /// Creates a unique temporary directory using the system's default location
    /// for temporary files. If a prefix is provided to the constructor, the
    /// directory is created in the path <temp dir>/prefix/. The prefix can be
    /// a path consisting of multiple segments.
    explicit directory(std::string_view prefix = "") {
        const auto parent = std::filesystem::temp_directory_path() / prefix;
        std::string arg = parent / "XXXXXX";

        std::filesystem::create_directories(parent);
        this->p = ::mkdtemp(arg.data());
    }

    /// Creates a directory from a moved @p other
    directory(directory&& other) noexcept : p(std::move(other.p)) {
        other.p.clear();
    }

    /// Deletes this directory recursively and assigns to it a moved @p other
    directory& operator=(directory&& other) noexcept {
        this->remove();
        this->p = std::move(other.p);
        other.p.clear();
        return *this;
    }

    /// Returns this directory path
    operator const std::filesystem::path&() const noexcept { return this->p; }

    /// Returns this directory path
    const std::filesystem::path& path() const noexcept { return this->p; }

    /// Concatenates this directory path with a given @p source
    std::filesystem::path operator/(std::string_view source) const {
        return this->p / source;
    }

    /// Provides access to this directory path members
    const std::filesystem::path* operator->() const noexcept {
        return std::addressof(this->p);
    }

    /// Deletes this directory recursively when the enclosing scope is exited
    ~directory() noexcept { this->remove(); }

    directory(const directory&) = delete;         ///< not copy-constructible
    auto operator=(const directory&) = delete;    ///< not copy-assignable
};

}    // namespace tmp
