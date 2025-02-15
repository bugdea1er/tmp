#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"
#include "move.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
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

/// Opens a file in read-only mode
/// @param[in]  path The path to the file to open
/// @param[out] ec   Parameter for error reporting
/// @returns A handle to the open file
file::native_handle_type open(const fs::path& path,
                              std::error_code& ec) noexcept {
  ec.clear();

#ifdef _WIN32
  HANDLE handle =
      CreateFile(path.c_str(), GENERIC_READ,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  int handle = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
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

/// Copies a file contents from the given path to the given file descriptor
/// @param[in]  from The path to the source file
/// @param[in]  to   The target file descriptor
/// @param[out] ec   Parameter for error reporting
void copy_file(const fs::path& from, file::native_handle_type to,
               std::error_code& ec) noexcept {
  // TODO: can be optimized using `sendfile`, `copyfile` or other system API
  file::native_handle_type source = open(from, ec);
  if (ec) {
    return;
  }

  buffer_type buffer = buffer_type();
  while (true) {
#ifdef _WIN32
    DWORD bytes_read;
    if (!ReadFile(source, buffer.data(), block_size, &bytes_read, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      break;
    }
#else
    ssize_t bytes_read = read(source, buffer.data(), block_size);
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

  close(source);
}
}    // namespace

file::file(std::pair<fs::path, filebuf> handle) noexcept
    : entry(std::move(handle.first)),
      std::iostream(std::addressof(sb)),
      sb(std::move(handle.second)) {}

file::file(std::ios::openmode mode)
    : file(create_file(mode)) {}

file file::copy(const fs::path& path, std::ios::openmode mode) {
  file tmpfile = file(mode);

  std::error_code ec;
  copy_file(path, tmpfile.native_handle(), ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

file::native_handle_type file::native_handle() const noexcept {
  return sb.native_handle();
}

void file::move(const fs::path& to) {
  sb.close();

  std::error_code ec;
  tmp::move(*this, to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary file", path(), to, ec);
  }

  entry::clear();
}

file::~file() noexcept {
  sb.close();
}

// NOLINTBEGIN(*-use-after-move)
file::file(file&& other) noexcept
    : entry(std::move(other)),
      std::iostream(std::move(other)),
      sb(std::move(other.sb)) {
  set_rdbuf(std::addressof(sb));
}

file& file::operator=(file&& other) {
  std::iostream::operator=(std::move(other));

  // The stream buffer must be assigned first to close the file;
  // otherwise `entry` may not be able to remove the file before reassigning
  sb = std::move(other.sb);
  entry::operator=(std::move(other));

  return *this;
}
// NOLINTEND(*-use-after-move)
}    // namespace tmp
