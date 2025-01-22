#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <sstream>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// Maximum number of bytes in one read/write operation
constexpr std::string_view::size_type io_max = std::numeric_limits<int>::max();

/// A block size for file reading
constexpr std::size_t block_size = 4096;

/// Reads the content from the given native handle
/// @note uses handle current offset for reading
/// @param[in]  handle A native handle to read from
/// @param[out] ec     Parameter for error reporting
/// @returns A string with read contents
/// @throws std::bad_alloc if memory allocation fails
std::string read(entry::native_handle_type handle, std::error_code& ec) {
  std::array buffer = std::array<std::string::value_type, block_size>();
  std::ostringstream content;

  while (true) {
#ifdef _WIN32
    DWORD read;
    if (!ReadFile(handle, buffer.data(), block_size, &read, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      return std::string();
    }
#else
    ssize_t read = ::read(handle, buffer.data(), block_size);
    if (read < 0) {
      ec = std::error_code(errno, std::system_category());
      return std::string();
    }
#endif
    if (read == 0) {
      break;
    }

    content.write(buffer.data(), read);
  }

  ec.clear();
  return std::move(content).str();
}

/// Writes the given content to the native handle
/// @note uses handle current offset for writing
/// @param[in]  handle  A native handle to write to
/// @param[in]  content A string to write to this file
/// @param[out] ec      Parameter for error reporting
void write(entry::native_handle_type handle, std::string_view content,
           std::error_code& ec) noexcept {
  do {
    int writable = static_cast<int>(std::min(content.size(), io_max));

#ifdef _WIN32
    DWORD written;
    if (!WriteFile(handle, content.data(), writable, &written, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      return;
    }
#else
    ssize_t written = ::write(handle, content.data(), writable);
    if (written < 0) {
      ec = std::error_code(errno, std::system_category());
      return;
    }
#endif

    content = content.substr(written);
  } while (!content.empty());
  ec.clear();

#ifdef _WIN32
  if (!FlushFileBuffers(handle)) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  if (fsync(handle) == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif
}

/// Opens a file for reading
/// @param [in]  path The file path
/// @param [out] ec   Parameter for error reporting
/// @returns A native handle to the opened file
entry::native_handle_type open(const fs::path& path, std::error_code& ec) {
#ifdef _WIN32
  HANDLE handle = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    ec = std::error_code(GetLastError(), std::system_category());
    return entry::native_handle_type();
  }
#else
  int handle = ::open(path.c_str(), O_RDONLY);
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
    return entry::native_handle_type();
  }
#endif

  ec.clear();
  return handle;
}
}    // namespace

file::file(std::string_view label, std::string_view extension)
    : entry(create_file(label, extension)),
      binary(true) {}

file::file(std::error_code& ec)
    : entry(create_file("", "", ec)),
      binary(true) {}

file::file(std::string_view label, std::error_code& ec)
    : entry(create_file(label, "", ec)),
      binary(true) {}

file::file(std::string_view label, std::string_view extension,
           std::error_code& ec)
    : entry(create_file(label, extension, ec)),
      binary(true) {}

file file::text(std::string_view label, std::string_view extension) {
  file result = file(label, extension);
  result.binary = false;

  return result;
}

file file::text(std::error_code& ec) {
  return text("", ec);
}

file file::text(std::string_view label, std::error_code& ec) {
  return text(label, "", ec);
}

file file::text(std::string_view label, std::string_view extension,
                std::error_code& ec) {
  file result = file(label, extension, ec);
  result.binary = false;

  return result;
}

file file::copy(const fs::path& path, std::string_view label) {
  std::error_code ec;
  file copy = file::copy(path, label, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return copy;
}

file file::copy(const fs::path& path, std::error_code& ec) {
  return copy(path, "", ec);
}

file file::copy(const fs::path& path, std::string_view label,
                std::error_code& ec) {
  file copy = file(label, path.extension().native(), ec);
  if (ec) {
    return copy;
  }

  native_handle_type handle = open(path.c_str(), ec);
  if (ec) {
    return copy;
  }

  std::string content = tmp::read(handle, ec);
  if (!ec) {
    tmp::write(copy.native_handle(), content, ec);
  }

  return copy;
}

std::uintmax_t file::size() const {
  std::error_code ec;
  std::uintmax_t result = size(ec);

  if (ec) {
    throw fs::filesystem_error("Cannot get a temporary file size", path(), ec);
  }

  return result;
}

std::uintmax_t file::size(std::error_code& ec) const noexcept {
#ifdef _WIN32
  DWORD size_upper;
  DWORD size_lower = GetFileSize(native_handle(), &size_upper);
  if (size_upper == INVALID_FILE_SIZE) {
    ec = std::error_code(GetLastError(), std::system_category());
    return static_cast<std::uintmax_t>(-1);
  }

  std::uintmax_t size = size_upper;
  return size << sizeof(DWORD) * CHAR_BIT | size_lower;
#else
  struct stat stat;
  if (fstat(native_handle(), &stat) == -1) {
    ec = std::error_code(errno, std::system_category());
    return static_cast<std::uintmax_t>(-1);
  }

  return stat.st_size;
#endif
}

std::string file::read() const {
  std::error_code ec;
  std::string content = read(ec);

  if (ec) {
    throw fs::filesystem_error("Cannot read a temporary file", path(), ec);
  }

  return content;
}

std::string file::read(std::error_code& ec) const {
#ifdef _WIN32    // TODO: can be optimized to not open the file again
  if (!binary) {
    try {
      std::ifstream stream = input_stream();
      stream.exceptions(std::ios::failbit | std::ios::badbit);

      return std::string(std::istreambuf_iterator(stream), {});
    } catch (const std::ios::failure& err) {
      ec = err.code();
      return std::string();
    }
  }
#endif
  native_handle_type handle = native_handle();

#ifdef _WIN32
  if (SetFilePointer(handle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  if (lseek(handle, 0, SEEK_SET) == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    return std::string();
  }

  return tmp::read(handle, ec);
}

void file::write(std::string_view content) const {
  std::error_code ec;
  write(content, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot write to a temporary file", path(), ec);
  }
}

void file::write(std::string_view content, std::error_code& ec) const {
  native_handle_type handle = native_handle();

#ifdef _WIN32
  if (SetFilePointer(handle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
      !SetEndOfFile(handle)) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  if (ftruncate(handle, 0) == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (!ec) {
    append(content, ec);
  }
}

void file::append(std::string_view content) const {
  std::error_code ec;
  append(content, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot append to a temporary file", path(), ec);
  }
}

void file::append(std::string_view content, std::error_code& ec) const {
#ifdef _WIN32    // TODO: can be optimized to not open the file again
  if (!binary) {
    try {
      std::ofstream stream = output_stream(std::ios::app);
      stream.exceptions(std::ios::failbit | std::ios::badbit);

      stream << content;
    } catch (const std::ios::failure& err) {
      ec = err.code();
    }

    return;
  }
#endif
  native_handle_type handle = native_handle();

#ifdef _WIN32
  if (SetFilePointer(handle, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  if (lseek(handle, 0, SEEK_END) == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (!ec) {
    tmp::write(handle, content, ec);
  }
}

std::ifstream file::input_stream() const {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();
  return std::ifstream(path(), mode);
}

std::ofstream file::output_stream(std::ios::openmode mode) const {
  binary ? mode |= std::ios::binary : mode &= ~std::ios::binary;
  return std::ofstream(path(), mode);
}

file::~file() noexcept = default;

file::file(file&&) noexcept = default;
file& file::operator=(file&& other) noexcept = default;
}    // namespace tmp

std::size_t
std::hash<tmp::file>::operator()(const tmp::file& file) const noexcept {
  return std::hash<tmp::entry>()(file);
}
