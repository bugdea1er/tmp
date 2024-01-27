#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {

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
    std::filesystem::path underlying;    ///< This file path

    /// Creates a unique temporary path using the given constructor function
    template<typename C>
    explicit path(std::string_view prefix, C constructor) {
        const auto parent = std::filesystem::temp_directory_path() / prefix;
        std::string arg = parent / "XXXXXX";

        std::filesystem::create_directories(parent);
        constructor(arg.data());
        this->underlying = arg;
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
