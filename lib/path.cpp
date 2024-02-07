#include <tmp/path>

#include <filesystem>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace {

/// Options for recursive overwriting coping
const fs::copy_options copy_options = fs::copy_options::recursive
                                    | fs::copy_options::overwrite_existing;

/// Creates the parent directory of the given path if it does not exist
/// @param path The path for which the parent directory needs to be created
/// @throws fs::filesystem_error if cannot create the parent
void create_parent(const fs::path& path) {
    fs::create_directories(path.parent_path());
}

/// Deletes the given path recursively, ignoring any errors
/// @param path The path of the directory to remove
void remove(const tmp::path& path) noexcept {
    if (!path->empty()) {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
}

/// Throws a filesystem error indicating that a temporary resource cannot be
/// moved to the specified path
/// @param to   The target path where the resource was intended to be moved
/// @param ec   The error code associated with the failure to move the resource
/// @throws fs::filesystem_error when called
[[noreturn]] void throw_move_error(const fs::path& to, std::error_code ec) {
    throw fs::filesystem_error("Cannot move temporary resource", to, ec);
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
    create_parent(to);

    std::error_code ec;
    fs::rename(*this, to, ec);
    if (ec == std::errc::cross_device_link) {
        if (fs::is_regular_file(*this) && fs::is_directory(to)) {
            ec = std::make_error_code(std::errc::is_a_directory);
            throw_move_error(to, ec);
        }

        fs::remove_all(to);
        fs::copy(*this, to, copy_options, ec);
    }

    if (ec) {
        throw_move_error(to, ec);
    }

    remove(*this);
    release();
}

fs::path path::make_pattern(std::string_view prefix) {
    fs::path pattern = fs::temp_directory_path() / prefix / "XXXXXX";
    create_parent(pattern);

    return pattern;
}
}    // namespace tmp
