#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {

/// The tmp::path class is a smart handle that owns and manages a temporary path
/// and disposes of it when this handle goes out of scope.
///
/// Subclass this and provide mktemp-like function to the constructor to create
/// temporary files and directories.
class path {
public:
    /// Creates a path from a moved @p other
    path(path&& other) noexcept : underlying(std::move(other.underlying)) {
        other.underlying.clear();
    }

    /// Deletes this path and assigns to it a moved @p other
    path& operator=(path&& other) noexcept {
        this->remove();
        this->underlying = std::move(other.underlying);
        other.underlying.clear();
        return *this;
    }

    path(const path&) = delete;              ///< not copy-constructible
    auto operator=(const path&) = delete;    ///< not copy-assignable

    /// Deletes this path recursively when the enclosing scope is exited
    virtual ~path() noexcept { this->remove(); }

    /// Returns the underlying path
    operator const std::filesystem::path&() const noexcept {
        return this->underlying;
    }

    /// Provides access to the underlying path members
    const std::filesystem::path* operator->() const noexcept {
        return std::addressof(this->underlying);
    }

protected:
    /// Exception type that should be used by subclasses to signal errors
    using error = std::filesystem::filesystem_error;

    std::filesystem::path underlying;    ///< This file path

    /// Creates a unique temporary path using the given constructor function.
    /// @param prefix the path between system temp
    /// @param creator wrapped mktemp-like function that returns resulting path
    explicit path(std::filesystem::path path) : underlying(std::move(path)) {}

    static std::filesystem::path make_parent(std::string_view prefix) {
        const auto parent = std::filesystem::temp_directory_path() / prefix;
        std::filesystem::create_directories(parent);

        return parent;
    }

private:
    /// Deletes this path recursively, ignoring any errors
    void remove() const noexcept {
        if (!this->underlying.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(*this, ec);
        }
    }
};

}    // namespace tmp
