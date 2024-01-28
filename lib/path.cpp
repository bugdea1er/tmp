#include <tmp/path.hpp>

#include <system_error>
#include <utility>

namespace {

/// Deletes the given path recursively, ignoring any errors.
/// If the path is empty, does nothing.
/// @param path The path of the directory to remove
void remove(const tmp::path& path) noexcept {
    if (!path->empty()) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
}

}    // namespace

namespace tmp {

path::path(path&& other) noexcept : underlying(std::move(other.underlying)) {
    other.underlying.clear();
}

path& path::operator=(path&& other) noexcept {
    remove(*this);
    this->underlying = std::move(other.underlying);
    other.underlying.clear();
    return *this;
}

path::~path() noexcept {
    remove(*this);
}

path::operator const std::filesystem::path&() const noexcept {
    return this->underlying;
}

const std::filesystem::path* path::operator->() const noexcept {
    return std::addressof(this->underlying);
}

path::path(std::filesystem::path path) : underlying(std::move(path)) {
}

std::string path::make_pattern(std::string_view prefix) {
    const auto parent = std::filesystem::temp_directory_path() / prefix;
    std::filesystem::create_directories(parent);

    return parent / "XXXXXX";
}

}    // namespace tmp
