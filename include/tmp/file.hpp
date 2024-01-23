#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace tmp {

/// File is a smart handle that owns and manages a temporary file and
/// disposes of it when this handle goes out of scope.
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
    /// Creates a unique temp file with the given @p prefix
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

    /// Deletes this file when the enclosing scope is exited
    ~file() noexcept { this->remove(); }

    file(const file&) = delete;              ///< not copy-constructible
    auto operator=(const file&) = delete;    ///< not copy-assignable
};

}    // namespace tmp
