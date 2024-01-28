#pragma once

#include <tmp/path.hpp>

#include <cstdlib>
#include <fstream>
#include <string_view>

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
class file final : public path {
public:
    /// Creates a unique temporary binary file using the system's default
    /// location for temporary files. If a prefix is provided to the
    /// constructor, the directory is created in the path <temp dir>/prefix/.
    /// The prefix can be a path consisting of multiple segments.
    file(std::string_view prefix = "") : file(prefix, /*binary=*/true) {}

    /// Creates a unique temporary text file using the system's default location
    /// for temporary files. If a prefix is provided to the constructor, the
    /// directory is created in the path <temp dir>/prefix/. The prefix can be
    /// a path consisting of multiple segments.
    static file text(std::string_view prefix = "") {
        return file(prefix, /*binary=*/false);
    }

    /// Writes the given @p content to this file discarding any previous content
    void write(std::string_view content) const {
        this->stream(/*append=*/false) << content;
    }

    /// Appends the given @p content to the end of this file
    void append(std::string_view content) const {
        this->stream(/*append=*/true) << content;
    }

    /// Deletes this file when the enclosing scope is exited
    ~file() noexcept override = default;

    file(file&&) noexcept = default;               ///< move-constructible
    file& operator=(file&&) noexcept = default;    ///< move-assignable
    file(const file&) = delete;                    ///< not copy-constructible
    auto operator=(const file&) = delete;          ///< not copy-assignable

private:
    bool binary;    ///< This file write mode

    /// Creates a unique temporary file using the system's default location
    /// for temporary files. If a prefix is provided to the constructor, the
    /// directory is created in the path <temp dir>/prefix/. The prefix can be
    /// a path consisting of multiple segments.
    explicit file(std::string_view prefix, bool binary) : path(prefix, creator),
                                                          binary(binary) {}

    /// Creates a unique temporary file based on the given @p pattern path.
    /// The parent path of the given argument must exist.
    static std::string creator(std::string path) {
        if (mkstemp(path.data()) == -1) {
            auto ec = std::error_code(errno, std::system_category());
            throw error("Cannot create temporary file", ec);
        }

        return path;
    }

    /// Returns a stream for this file
    std::ofstream stream(bool append) const noexcept {
        std::ios::openmode mode = append ? std::ios::app : std::ios::trunc;
        return this->binary
            ? std::ofstream { this->underlying, mode | std::ios::binary }
            : std::ofstream { this->underlying, mode };
    }
};

}    // namespace tmp
