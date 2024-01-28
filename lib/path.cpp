#include <tmp/path.hpp>

namespace tmp {

namespace {

/// Deletes the given path recursively, ignoring any errors
void remove(const tmp::path& path) noexcept {
    if (!path->empty()) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
}

}    // namespace

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

/// Creates a unique temporary path using the given constructor function.
/// @param prefix the path between system temp
/// @param creator wrapped mktemp-like function that returns resulting path
path::path(std::filesystem::path path) : underlying(std::move(path)) {
}

/// Creates a pattern for the mktemp-like functions.
/// If @p prefix is not empty, it is appended to the tempdir
std::string path::make_pattern(std::string_view prefix) {
    const auto parent = std::filesystem::temp_directory_path() / prefix;
    std::filesystem::create_directories(parent);

    return parent / "XXXXXX";
}

}    // namespace tmp
