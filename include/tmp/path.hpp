#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {

class path {
protected:
    std::filesystem::path p;    ///< This file path

    void remove() const noexcept {
        if (!this->p.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(this->p, ec);
        }
    }

public:
    template<typename C>
    explicit path(std::string_view prefix, C constructor) {
        const auto parent = std::filesystem::temp_directory_path() / prefix;
        std::string arg = parent / "XXXXXX";

        std::filesystem::create_directories(parent);
        constructor(arg.data());
        this->p = arg;
    }

    /// Creates a path from a moved @p other
    path(path&& other) noexcept: p(std::move(other.p)) {
        other.p.clear();
    }

    /// Deletes this path and assigns to it a moved @p other
    path& operator=(path&& other) noexcept {
        this->remove();
        this->p = std::move(other.p);
        other.p.clear();
        return *this;
    }

    path(const path&) = delete;              ///< not copy-constructible
    auto operator=(const path&) = delete;    ///< not copy-assignable

    /// Deletes this path when the enclosing scope is exited
    ~path() noexcept {
        this->remove();
    }

    operator const std::filesystem::path&() const noexcept {
        return this->p;
    }

    const std::filesystem::path* operator->() const noexcept {
        return std::addressof(this->p);
    }
};

}    // namespace tmp
