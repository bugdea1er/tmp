#include <tmp/path>

#include <filesystem>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace {

/// Options for recursive overwriting coping
const fs::copy_options copy_options = fs::copy_options::recursive
                                    | fs::copy_options::overwrite_existing;

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
    : underlying(other.release()) {}

path& path::operator=(path&& other) noexcept {
    remove(*this);
    underlying = other.release();
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

fs::path path::release() noexcept {
    fs::path path = std::move(underlying);
    underlying.clear();
    return path;
}

void path::move(const fs::path& to) {
    std::error_code ec;
    fs::rename(*this, to, ec);
    if (ec == std::errc::cross_device_link) {
        fs::copy(*this, to, copy_options, ec);
    }

    if (ec) {
        throw fs::filesystem_error("Cannot move temporary resource", to, ec);
    }

    remove(*this);
    release();
}

fs::path path::make_pattern(std::string_view prefix) {
    fs::path parent = fs::temp_directory_path() / prefix;
    fs::create_directories(parent);

    return parent / "XXXXXX";
}
}    // namespace tmp
