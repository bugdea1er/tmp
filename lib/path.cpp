#include <tmp/path.hpp>

#include <filesystem>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace {

/// Deletes the given path recursively, ignoring any errors
/// @param path The path of the directory to remove
void remove(const tmp::path& path) noexcept {
    if (!path->empty()) {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
}
}    // namespace

namespace tmp {

path::path(fs::path path)
    : underlying(std::move(path)) {}

path::path(path&& other) noexcept
    : underlying(std::move(other.underlying)) {
    other.underlying.clear();
}

path& path::operator=(path&& other) noexcept {
    remove(*this);
    underlying = std::move(other.underlying);
    other.underlying.clear();
    return *this;
}

path::~path() noexcept {
    remove(*this);
}

path::operator const fs::path&() const noexcept {
    return underlying;
}

const fs::path* path::operator->() const noexcept {
    return std::addressof(underlying);
}

std::string path::make_pattern(std::string_view prefix) {
    fs::path parent = fs::temp_directory_path() / prefix;
    fs::create_directories(parent);

    return parent / "XXXXXX";
}
}    // namespace tmp
