#include <tmp/directory>
#include <tmp/file>
#include <tmp/filesystem>
#include <tmp/path>

#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef WIN32
#include <windows.h>
#define CHARS_IN_GUID 39
#else
#include <unistd.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Options for recursive overwriting copying
const fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;

/// Creates the parent directory of the given path if it does not exist
/// @param path The path for which the parent directory needs to be created
/// @throws fs::filesystem_error if cannot create the parent of the given path
void create_parent(const fs::path& path) {
    fs::create_directories(path.parent_path());
}

/// Deletes the given path recursively, ignoring any errors
/// @param path The path to remove recursively
void remove(const path& path) noexcept {
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

/// Creates a temporary path pattern with the given prefix and suffix
///
/// The parent of the resulting path is created when this function is called
/// @param prefix   A prefix to be used in the path pattern
/// @param suffix   A suffix to be used in the path pattern
/// @returns A path pattern for the unique temporary path
/// @throws fs::filesystem_error if cannot create the parent of the path pattern
fs::path make_pattern(std::string_view prefix, std::string_view suffix) {
#ifdef WIN32
    GUID guid;
    CoCreateGuid(&guid);

    wchar_t name[CHARS_IN_GUID];
    swprintf(name, CHARS_IN_GUID,
             L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
             guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
             guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
             guid.Data4[6], guid.Data4[7]);
#else
    char name[] = "XXXXXX";
#endif
    fs::path pattern = filesystem::root() / prefix / name;

    pattern += suffix;
    create_parent(pattern);

    return pattern;
}

/// Creates a temporary file with the given prefix in the system's
/// temporary directory, and returns its path
/// @param prefix   The prefix to use for the temporary file name
/// @param suffix   The suffix to use for the temporary file name
/// @returns A path to the created temporary file
/// @throws fs::filesystem_error if cannot create the temporary file
fs::path create_file(std::string_view prefix, std::string_view suffix) {
    fs::path::string_type path = make_pattern(prefix, suffix);
    std::error_code ec;
#ifdef WIN32
    HANDLE file =
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) {
            return create_file(prefix, suffix);
        }

        ec = std::error_code(err, std::system_category());
    }
#else
    if (mkstemps(path.data(), suffix.size()) == -1) {
        ec = std::error_code(errno, std::system_category());
    }
#endif
    if (ec) {
        throw fs::filesystem_error("Cannot create temporary file", ec);
    }

    return path;
}

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory, and returns its path
/// @param prefix   The prefix to use for the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error if cannot create the temporary directory
fs::path create_directory(std::string_view prefix) {
    fs::path::string_type path = make_pattern(prefix, "");
    std::error_code ec;
#ifdef WIN32
    if (!CreateDirectoryW(path.c_str(), nullptr)) {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) {
            return create_directory(prefix);
        }

        ec = std::error_code(err, std::system_category());
    }
#else
    if (mkdtemp(path.data()) == nullptr) {
        ec = std::error_code(errno, std::system_category());
    }
#endif
    if (ec) {
        throw fs::filesystem_error("Cannot create temporary directory", ec);
    }

    return path;
}

/// Opens a temporary file for writing and returns an output file stream
/// @param file     The file to open
/// @param binary   Whether to open the file in binary mode
/// @param append   Whether to append to the end of the file
/// @returns An output file stream
std::ofstream stream(const file& file, bool binary, bool append) noexcept {
    std::ios::openmode mode = append ? std::ios::app : std::ios::trunc;
    if (binary) {
        mode |= std::ios::binary;
    }

    return std::ofstream(static_cast<const fs::path&>(file), mode);
}
}    // namespace

//===----------------------------------------------------------------------===//
// tmp::path implementation
//===----------------------------------------------------------------------===//

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

//===----------------------------------------------------------------------===//
// tmp::file implementation
//===----------------------------------------------------------------------===//

file::file(std::string_view prefix, std::string_view suffix)
    : file(prefix, suffix, /*binary=*/true) {}

file::file(std::string_view prefix, std::string_view suffix, bool binary)
    : path(create_file(prefix, suffix)),
      binary(binary) {}

file file::text(std::string_view prefix, std::string_view suffix) {
    return file(prefix, suffix, /*binary=*/false);
}

file file::copy(const fs::path& path, std::string_view prefix,
                std::string_view suffix) {
    file tmpfile = file(prefix, suffix);
    fs::copy(path, tmpfile, copy_options);
    return tmpfile;
}

std::string file::read() const {
    const fs::path& file = *this;
    std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();

    std::ifstream stream = std::ifstream(file, mode);
    return std::string(std::istreambuf_iterator<char>(stream), {});
}

void file::write(std::string_view content) const {
    stream(*this, binary, /*append=*/false) << content;
}

void file::append(std::string_view content) const {
    stream(*this, binary, /*append=*/true) << content;
}

file::~file() noexcept = default;

file::file(file&&) noexcept = default;
file& file::operator=(file&&) noexcept = default;

//===----------------------------------------------------------------------===//
// tmp::directory implementation
//===----------------------------------------------------------------------===//

directory::directory(std::string_view prefix)
    : path(create_directory(prefix)) {}

directory directory::copy(const fs::path& path, std::string_view prefix) {
    if (fs::is_regular_file(path)) {
        std::error_code ec = std::make_error_code(std::errc::is_a_directory);
        throw fs::filesystem_error("Cannot copy temporary directory", ec);
    }

    directory tmpdir = directory(prefix);
    fs::copy(path, tmpdir, copy_options);
    return tmpdir;
}

fs::path directory::operator/(std::string_view source) const {
    return static_cast<const fs::path&>(*this) / source;
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;

//===----------------------------------------------------------------------===//
// tmp::fs implementation
//===----------------------------------------------------------------------===//

fs::path filesystem::root(std::string_view prefix) {
    return fs::temp_directory_path() / prefix;
}

fs::space_info filesystem::space() {
    return fs::space(root());
}
}    // namespace tmp
