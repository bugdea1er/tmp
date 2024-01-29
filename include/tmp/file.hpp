#pragma once

#include <tmp/path.hpp>

#include <filesystem>
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
    /// Creates a unique temporary binary file
    ///
    /// The file path consists of the system's temporary directory path, the
    /// given prefix, and six random characters to ensure path uniqueness
    ///
    /// @param prefix   A prefix to be used in the temporary file path
    /// @throws std::filesystem::filesystem_error if cannot create a file
    explicit file(std::string_view prefix = "");

    /// Creates a unique temporary text file
    ///
    /// The file path consists of the system's temporary directory path, the
    /// given prefix, and six random characters to ensure path uniqueness
    ///
    /// @param prefix   A prefix to be used in the temporary file path
    /// @throws std::filesystem::filesystem_error if cannot create a file
    static file text(std::string_view prefix = "");

    /// Writes the given content to this file discarding any previous content
    /// @param content  A string to write to this file
    void write(std::string_view content) const;

    /// Appends the given content to the end of this file
    /// @param content  A string to append to this file
    void append(std::string_view content) const;

    /// Deletes the managed file if its path is not empty
    ~file() noexcept override;

    file(file&&) noexcept;                   ///< move-constructible
    file& operator=(file&&) noexcept;        ///< move-assignable
    file(const file&) = delete;              ///< not copy-constructible
    auto operator=(const file&) = delete;    ///< not copy-assignable

private:
    /// Whether the managed file is opened in binary write mode
    bool binary;

    /// Creates a unique temporary file
    ///
    /// The file path consists of the system's temporary directory path, the
    /// given prefix, and six random characters to ensure path uniqueness
    ///
    /// @param prefix   A prefix to be used in the temporary file path
    /// @param binary   Whether the managed file is opened in binary write mode
    /// @throws std::filesystem::filesystem_error if cannot create a file
    explicit file(std::string_view prefix, bool binary);
};

}    // namespace tmp
