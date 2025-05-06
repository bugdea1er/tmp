#include "abi.hpp"
#include "create.hpp"

#include <tmp/file>

#include <array>
#include <climits>
#include <cstddef>
#include <filesystem>
#include <ios>
#include <istream>
#include <memory>
#include <system_error>
#include <utility>

#if __has_include(<copyfile.h>)
#include <copyfile.h>
#elif __has_include(<sys/sendfile.h>)
#include <sys/sendfile.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#include <corecrt_io.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// RAII wrapper for native open file descriptor
/// @note closes the file in destructor
class file_handle {
public:
  /// Opens a file for reading
  /// @param[in]  path The path to the file to open
  /// @param[out] ec   Parameter for error reporting
  file_handle(const fs::path& path, std::error_code& ec)
#ifdef _WIN32
      : handle(
            CreateFile(path.c_str(), GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr))
#else
      : handle(open(path.c_str(), O_RDONLY | O_NONBLOCK))
#endif
  {
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE) {
      ec = std::error_code(GetLastError(), std::system_category());
      return;
    }

    BY_HANDLE_FILE_INFORMATION handle_info;
    if (GetFileInformationByHandle(handle, &handle_info) == 0) {
      ec = std::error_code(GetLastError(), std::system_category());
      return;
    }

    // Based on Microsoft's C++ STL implementation: If a file is not a reparse
    // point and is not a directory, then it is considered a regular file
    // https://github.com/microsoft/STL/blob/main/stl/inc/filesystem#L1982
    if ((handle_info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ||
        (handle_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      ec = std::make_error_code(std::errc::not_supported);
      return;
    }
#else
    if (handle == -1) {
      ec = std::error_code(errno, std::system_category());
      return;
    }

    struct stat file_stat;
    if (fstat(handle, &file_stat) == -1) {
      ec = std::error_code(errno, std::system_category());
      return;
    }

    if ((file_stat.st_mode & S_IFMT) != S_IFREG) {
      ec = std::make_error_code(std::errc::not_supported);
      return;
    }
#endif

    ec.clear();
  }

  /// Returns the underlying file handle
  operator file::native_handle_type() const noexcept {
    return handle;
  }

  /// Closes the file, ignoring any errors
  ~file_handle() noexcept {
#ifdef _WIN32
    if (handle != INVALID_HANDLE_VALUE) {
      CloseHandle(handle);
    }
#else
    if (handle != 1) {
      close(handle);
    }
#endif
  }

  file_handle(file_handle&&) noexcept = delete;
  file_handle& operator=(file_handle&&) = delete;
  file_handle(const file_handle&) = delete;
  file_handle& operator=(const file_handle&) = delete;

private:
  /// Returns the underlying file handle
  file::native_handle_type handle;
};

#if __has_include(<copyfile.h>)
/// Copies file contents from one file descriptor to another
/// @param[in]  from The source file descriptor
/// @param[in]  to   The target file descriptor
/// @param[out] ec   Parameter for error reporting
void copy_file(file::native_handle_type from, file::native_handle_type to,
               std::error_code& ec) noexcept {
  if (fcopyfile(from, to, nullptr, COPYFILE_DATA) != 0) {
    ec = std::error_code(errno, std::system_category());
    return;
  }

  ec.clear();
}
#elif __has_include(<sys/sendfile.h>)
/// Copies file contents from one file descriptor to another
/// @param[in]  from The source file descriptor
/// @param[in]  to   The target file descriptor
/// @param[out] ec   Parameter for error reporting
void copy_file(file::native_handle_type from, file::native_handle_type to,
               std::error_code& ec) noexcept {
  while (true) {
    ssize_t res = sendfile(to, from, nullptr, INT_MAX);
    if (res == -1) {
      ec = std::error_code(errno, std::system_category());
      return;
    }

    if (res == 0) {
      break;
    }
  }

  ec.clear();
}
#else
/// Copies file contents from one file descriptor to another
/// @param[in]  from The source file descriptor
/// @param[in]  to   The target file descriptor
/// @param[out] ec   Parameter for error reporting
void copy_file(file::native_handle_type from, file::native_handle_type to,
               std::error_code& ec) noexcept {
  constexpr std::size_t blk_size = 4096;
  std::array<std::byte, blk_size> buffer = std::array<std::byte, blk_size>();

  while (true) {
#ifdef _WIN32
    DWORD bytes_read;
    if (!ReadFile(from, buffer.data(), blk_size, &bytes_read, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      break;
    }
#else
    ssize_t bytes_read = read(from, buffer.data(), blk_size);
    if (bytes_read < 0) {
      ec = std::error_code(errno, std::system_category());
      break;
    }
#endif
    if (bytes_read == 0) {
      break;
    }

#ifdef _WIN32
    DWORD written;
    if (!WriteFile(to, buffer.data(), bytes_read, &written, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      return;
    }
#else
    ssize_t written = write(to, buffer.data(), bytes_read);
    if (written < 0) {
      ec = std::error_code(errno, std::system_category());
      break;
    }
#endif
  }

  ec.clear();
}
#endif
}    // namespace

file::file(std::ios::openmode mode)
    : std::iostream(std::addressof(sb))
#if defined(_MSC_VER)
      ,
      underlying(create_file(mode), &std::fclose),
      sb(underlying.get())
#elif defined(_LIBCPP_VERSION)
      ,
      handle(create_file())
#endif
{
#ifndef _MSC_VER
  mode |= std::ios::in | std::ios::out;

#if defined(__GLIBCXX__)
  int handle = create_file();
  sb = __gnu_cxx::stdio_filebuf<char>(handle, mode);
#elif defined(_LIBCPP_VERSION)
  sb.__open(handle, mode);
#endif
#endif

  if (!sb.is_open()) {
#ifndef _WIN32
    close(handle);
#endif
    throw std::invalid_argument(
        "Cannot create a temporary file: invalid openmode");
  }
}

file file::copy(const fs::path& path, std::ios::openmode mode) {
  file tmpfile = file(mode);

  std::error_code ec;
  file_handle source = file_handle(path, ec);
  if (!ec) {
    copy_file(source, tmpfile.native_handle(), ec);
  }

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

file::native_handle_type file::native_handle() const noexcept {
#if defined(__GLIBCXX__)
  return sb.fd();
#elif defined(_LIBCPP_VERSION)
  return handle;
#else    // MSVC
  return reinterpret_cast<void*>(_get_osfhandle(_fileno(underlying.get())));
#endif
}

file::~file() noexcept = default;

// NOLINTBEGIN(*-use-after-move)
file::file(file&& other) noexcept
    : std::iostream(std::move(other)),
#if defined(_MSC_VER)
      underlying(std::move(other.underlying)),
#elif defined(_LIBCPP_VERSION)
      handle(other.handle),
#endif
      sb(std::move(other.sb)) {
  set_rdbuf(std::addressof(sb));
}
// NOLINTEND(*-use-after-move)

file& file::operator=(file&& other) = default;
}    // namespace tmp
