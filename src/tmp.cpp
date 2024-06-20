#include <tmp/directory>
#include <tmp/file>
#include <tmp/filesystem>
#include <tmp/path>

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef WIN32
#include <windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;

/// Creates the parent directory of the given path if it does not exist
/// @param[in]  path The path for which the parent directory needs to be created
/// @param[out] ec   Parameter for error reporting
/// @returns @c true if a parent directory was newly created, @c false otherwise
/// @throws std::bad_alloc if memory allocation fails
bool create_parent(const fs::path& path, std::error_code& ec) {
    return fs::create_directories(path.parent_path(), ec);
}

/// Creates a temporary path pattern with the given prefix and suffix
/// @param prefix   A prefix to be used in the path pattern
/// @param suffix   A suffix to be used in the path pattern
/// @returns A path pattern for the unique temporary path
/// @throws std::bad_alloc if memory allocation fails
fs::path make_pattern(std::string_view prefix, std::string_view suffix) {
#ifdef WIN32
    constexpr static std::size_t CHARS_IN_GUID = 39;
    GUID guid;
    CoCreateGuid(&guid);

    wchar_t name[CHARS_IN_GUID];
    swprintf(name, CHARS_IN_GUID,
             L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
             guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
             guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
             guid.Data4[6], guid.Data4[7]);
#else
    std::string_view name = "XXXXXX";
#endif
    fs::path pattern = filesystem::root(prefix) / name;

    pattern += suffix;
    return pattern;
}

/// Creates a temporary file with the given prefix in the system's
/// temporary directory, and opens it for reading and writing
///
/// @param prefix   The prefix to use for the temporary file name
/// @param suffix   The suffix to use for the temporary file name
/// @returns A path to the created temporary file and a handle to it
/// @throws fs::filesystem_error if cannot create the temporary file
std::pair<fs::path, file::native_handle_type>
create_file(std::string_view prefix, std::string_view suffix) {
    fs::path::string_type path = make_pattern(prefix, suffix);

    std::error_code ec;
    create_parent(path, ec);
    if (ec) {
        throw fs::filesystem_error("Cannot create temporary file", ec);
    }

#ifdef WIN32
    HANDLE handle =
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_ALREADY_EXISTS) {
            return create_file(prefix, suffix);
        }

        ec = std::error_code(err, std::system_category());
    }
#else
    const int handle = mkstemps(path.data(), static_cast<int>(suffix.size()));
    if (handle == -1) {
        ec = std::error_code(errno, std::system_category());
    }
#endif

    if (ec) {
        throw fs::filesystem_error("Cannot create temporary file", ec);
    }

    return std::pair(path, handle);
}

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory, and returns its path
/// @param prefix   The prefix to use for the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error if cannot create the temporary directory
fs::path create_directory(std::string_view prefix) {
    fs::path::string_type path = make_pattern(prefix, "");

    std::error_code ec;
    create_parent(path, ec);
    if (ec) {
        throw fs::filesystem_error("Cannot create temporary directory", ec);
    }

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

    return std::ofstream(file.path(), mode);
}

/// Deletes the given path recursively, ignoring any errors
/// @param path     The path to delete
void remove(const fs::path& path) noexcept {
    if (!path.empty()) {
        try {
            std::error_code ec;
            fs::remove_all(path, ec);    // Can throw std::bad_alloc
        } catch (const std::bad_alloc& ex) {
            static_cast<void>(ex);
        }
    }
}

/// Closes the given file, ignoring any errors
/// @param file     The file to close
void close(const file& file) noexcept {
    if (!file.path().empty()) {
#ifdef WIN32
        CloseHandle(file.native_handle());
#else
        ::close(file.native_handle());
#endif
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
    if (file* file = dynamic_cast<class file*>(this)) {
        close(*file);
    }

    fs::path path = std::move(underlying);
    underlying.clear();

    return path;
}

void path::move(const fs::path& to) {
    std::error_code ec;
    create_parent(to, ec);
    if (ec) {
        throw_move_error(to, ec);
    }

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

    if (file* file = dynamic_cast<class file*>(this)) {
        close(*file);
    }

    remove(*this);
    release();
}

//===----------------------------------------------------------------------===//
// tmp::file implementation
//===----------------------------------------------------------------------===//

file::file(std::pair<fs::path, native_handle_type> handle, bool binary) noexcept
    : tmp::path(std::move(handle.first)),
      handle(handle.second),
      binary(binary) {}

file::file(std::string_view prefix, std::string_view suffix, bool binary)
    : file(create_file(prefix, suffix), binary) {}

file::file(std::string_view prefix, std::string_view suffix)
    : file(prefix, suffix, /*binary=*/true) {}

file file::text(std::string_view prefix, std::string_view suffix) {
    return file(prefix, suffix, /*binary=*/false);
}

file file::copy(const fs::path& path, std::string_view prefix,
                std::string_view suffix) {
    std::error_code ec;
    file tmpfile = file(prefix, suffix);

    fs::copy_file(path, tmpfile, copy_options, ec);

    if (ec) {
        throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
    }

    return tmpfile;
}

file::native_handle_type file::native_handle() const noexcept {
    return handle;
}

const fs::path& file::path() const noexcept {
    return *this;
}

std::string file::read() const {
    std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();
    std::ifstream stream = std::ifstream(path(), mode);

    return std::string(std::istreambuf_iterator<char>(stream), {});
}

void file::write(std::string_view content) const {
    stream(*this, binary, /*append=*/false) << content;
}

void file::append(std::string_view content) const {
    stream(*this, binary, /*append=*/true) << content;
}

file::~file() noexcept {
    close(*this);
}

file::file(file&&) noexcept = default;

file& file::operator=(file&& other) noexcept {
    close(*this);

    tmp::path::operator=(std::move(other));

    this->binary = other.binary; // NOLINT(bugprone-use-after-move)
    this->handle = other.handle; // NOLINT(bugprone-use-after-move)

    return *this;
};

//===----------------------------------------------------------------------===//
// tmp::directory implementation
//===----------------------------------------------------------------------===//

directory::directory(std::string_view prefix)
    : tmp::path(create_directory(prefix)) {}

directory directory::copy(const fs::path& path, std::string_view prefix) {
    std::error_code ec;
    directory tmpdir = directory(prefix);

    if (fs::is_directory(path)) {
        fs::copy(path, tmpdir, copy_options, ec);
    } else {
        ec = std::make_error_code(std::errc::not_a_directory);
    }

    if (ec) {
        throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
    }

    return tmpdir;
}

const fs::path& directory::path() const noexcept {
    return *this;
}

fs::path directory::operator/(std::string_view source) const {
    return path() / source;
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
