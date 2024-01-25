#pragma once

#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace tmp {

/// The tmp::file class is a smart handle that owns and manages a temporary file
/// and disposes of it when this handle goes out of scope. It simplifies the
/// process of creating and managing temporary files by ensuring that they are
/// properly cleaned up when they are no longer needed.
///
/// When a tmp::file object is created, it creates a temporary file using the
/// system's default location for temporary files. If a prefix is provided to
/// the constructor, the file is created in the path
/// <system's default location for temporary files>/prefix/. The prefix can be
/// a path consisting of multiple segments.
///
/// The tmp::file class also provides an additional write and append methods
/// which allow writing data to the temporary file.
///
/// When the object is destroyed, it deletes the temporary file.
///
/// @note The tmp::file class cannot be copied or moved, as it is designed to
/// manage a single temporary file. If you need to create multiple temporary
/// files, you should create multiple instances of the class.
///
/// @code{.cpp}
///   #include <tmp/file.hpp>
///
///   auto prepareFile(const std::string& content) {
///     tmp::file tmpfile { "org.example.product" };
///     tmpfile << content;
///
///     // use the temporary file without worrying about cleanup
///
///     // the temporary file is deleted when the tmp::file object goes out
///     // of scope and is destroyed
///  }
/// @endcode
///
/// The above example uses a tmp::file object to create a temporary file with
/// the product identifier prefix. When the function returns, the tmp::file
/// object goes out of scope and the temporary file is deleted.
class file {
    std::filesystem::path p;    ///< This file path

    /// Deletes this file
    void remove() const noexcept {
        if (!this->p.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(this->p, ec);
        }
    }

public:
    /// Creates a unique temporary file using the system's default location
    /// for temporary files. If a prefix is provided to the constructor, the
    /// directory is created in the path <temp dir>/prefix/. The prefix can be
    /// a path consisting of multiple segments.
    explicit file(std::string_view prefix = "") {
        const auto parent = std::filesystem::temp_directory_path() / prefix;
        std::string arg = parent / "XXXXXX";

        std::filesystem::create_directories(parent);
        ::mkstemp(arg.data());
        this->p = arg;
    }

    /// Creates a file from a moved @p other
    file(file&& other) noexcept : p(std::move(other.p)) {
        other.p.clear();
    }

    /// Deletes this file and assigns to it a moved @p other
    file& operator=(file&& other) noexcept {
        this->remove();
        this->p = std::move(other.p);
        other.p.clear();
        return *this;
    }

    /// Returns this file path
    operator const std::filesystem::path&() const noexcept { return this->p; }

    /// Returns this file path
    const std::filesystem::path& path() const noexcept { return this->p; }

    /// Provides access to this file path members
    const std::filesystem::path* operator->() const noexcept {
        return std::addressof(this->p);
    }

    /// Writes the given @p content to this file discarding any previous content
    void write(std::string_view content, std::ios_base::openmode mode = 0) {
        std::ofstream { this->path(), std::ios_base::trunc | mode } << content;
    }

    /// Appends the given @p content to the end of this file
    void append(std::string_view content, std::ios_base::openmode mode = 0) {
        std::ofstream { this->path(), std::ios_base::app | mode } << content;
    }

    /// Deletes this file when the enclosing scope is exited
    ~file() noexcept { this->remove(); }

    file(const file&) = delete;              ///< not copy-constructible
    auto operator=(const file&) = delete;    ///< not copy-assignable
};

}    // namespace tmp
