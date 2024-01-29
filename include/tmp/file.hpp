#pragma once

#include <tmp/path.hpp>

#include <filesystem>
#include <string_view>

namespace tmp {

/// tmp::file is a smart handle that owns and manages a temporary file and
/// deletes it when this handle goes out of scope
///
/// When a tmp::file object is created, it creates a unique temporary file using
/// the system's default location for temporary files; the path consists of the
/// system's temporary directory path, the given prefix, and random characters
/// to ensure path uniqueness
///
/// The managed file is deleted of when either of the following happens:
/// - the managing tmp::file object is destroyed
/// - the managing tmp::file object is assigned another path via operator=
///
/// tmp::file provides additional `write` and `append` methods
/// which allow writing data to the temporary file
///
/// The following example uses a tmp::file object to create a temporary file
/// and write a string to it; when the function returns, the tmp::file object
/// goes out of scope and the temporary file:
///
/// @code{.cpp}
///   #include <tmp/file.hpp>
///
///   auto func(std::string_view content) {
///     auto tmpfile = tmp::file("org.example.product");
///     tmpfile.write(content);
///
///     // the temporary file is deleted recursively when the
///     // tmp::file object goes out of scope and is destroyed
///   }
/// @endcode
class file final : public path {
public:
    /// Creates a unique temporary binary file
    ///
    /// The file path consists of the system's temporary directory path, the
    /// given prefix, and random characters to ensure path uniqueness
    ///
    /// @param prefix   A prefix to be used in the temporary file path
    /// @throws std::filesystem::filesystem_error if cannot create a file
    explicit file(std::string_view prefix = "");

    /// Creates a unique temporary text file
    ///
    /// The file path consists of the system's temporary directory path, the
    /// given prefix, and random characters to ensure path uniqueness
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
    /// given prefix, and random characters to ensure path uniqueness
    ///
    /// @param prefix   A prefix to be used in the temporary file path
    /// @param binary   Whether the managed file is opened in binary write mode
    /// @throws std::filesystem::filesystem_error if cannot create a file
    explicit file(std::string_view prefix, bool binary);
};

}    // namespace tmp
