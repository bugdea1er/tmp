#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace tmp {

/// Directory is a smart handle that owns and manages a temporary directory and
/// disposes of it when this handle goes out of scope.
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
    /// Creates a unique temp directory with the given @p prefix
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

    /// Deletes this directory recursively when the enclosing scope is exited
    ~directory() noexcept { this->remove(); }

    directory(const directory&) = delete;         ///< not copy-constructible
    auto operator=(const directory&) = delete;    ///< not copy-assignable
};

}    // namespace tmp
