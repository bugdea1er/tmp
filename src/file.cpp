#include "create.hpp"
#include "export.hpp"

#include <tmp/file>

#include <array>
#include <cstddef>
#include <filesystem>
#include <ios>
#include <istream>
#include <memory>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#include <corecrt_io.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// A block size for file reading
/// @note should always be less than INT_MAX
constexpr std::size_t block_size = 4096;

/// A type of buffer for I/O file operations
using buffer_type = std::array<std::byte, block_size>;

/// Opens a file and returns its handle
/// @param[in]  path     The path to the file to open
/// @param[in]  readonly Whether to open file only for reading
/// @param[out] ec       Parameter for error reporting
/// @returns A handle to the open file
file::native_handle_type open(const fs::path& path, bool readonly,
                              std::error_code& ec) noexcept {
  ec.clear();

#ifdef _WIN32
  DWORD access = readonly ? GENERIC_READ : GENERIC_WRITE;
  DWORD creation_disposition = readonly ? OPEN_EXISTING : CREATE_ALWAYS;
  DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

  HANDLE handle =
      CreateFile(path.c_str(), access, share_mode, nullptr,
                 creation_disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  constexpr mode_t mode = 0644;
  int oflag = readonly ? O_RDONLY | O_NONBLOCK : O_RDWR | O_TRUNC | O_CREAT;
  int handle = ::open(path.c_str(), oflag, mode);
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  return handle;
}

/// Closes the given handle, ignoring any errors
/// @param[in] handle The handle to close
void close(file::native_handle_type handle) noexcept {
#ifdef _WIN32
  CloseHandle(handle);
#else
  ::close(handle);
#endif
}

/// Copies file contents from one file descriptor to another
/// @param[in]  from The source file descriptor
/// @param[in]  to   The target file descriptor
/// @param[out] ec   Parameter for error reporting
void copy_file(file::native_handle_type from, file::native_handle_type to,
               std::error_code& ec) noexcept {
  // TODO: can be optimized using `sendfile`, `copyfile` or other system API
  buffer_type buffer = buffer_type();
  while (true) {
#ifdef _WIN32
    DWORD bytes_read;
    if (!ReadFile(from, buffer.data(), block_size, &bytes_read, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      break;
    }
#else
    ssize_t bytes_read = read(from, buffer.data(), block_size);
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
}

/// Copies file contents from the given path to the given file descriptor
/// @param[in] from The path to the source file
/// @param[in] to   The target file descriptor
/// @throws std::filesystem::filesystem_error if cannot copy the file
void copy(const fs::path& from, file::native_handle_type to) {
  std::error_code ec;
  file::native_handle_type source = open(from, /*readonly=*/true, ec);
  if (!ec) {
    copy_file(source, to, ec);
    close(source);
  }

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", from, ec);
  }
}

/// Moves the file known by the given file descriptor to the given path
/// @param[in] from The file target descriptor
/// @param[in] to   The path to the target file
/// @throws std::filesystem::filesystem_error if cannot move the file
void move(file::native_handle_type from, const fs::path& to) {
  // FIXME: I couldn't figure out how to create a hard link to a file without
  //        other hard links, so I just copy it even within the same file system

  std::error_code ec;
  file::native_handle_type target = open(to, /*readonly=*/false, ec);
  if (!ec) {
    copy_file(from, target, ec);
    close(target);
  }

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary file", to, ec);
  }
}
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
  tmp::copy(path, tmpfile.native_handle());

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

void file::move(const fs::path& to) {
  seekg(0, std::ios::beg);
  tmp::move(native_handle(), to);
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
