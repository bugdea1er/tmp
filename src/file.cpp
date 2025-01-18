#include <tmp/entry>
#include <tmp/file>

#include "utils.hpp"

#include <array>
#include <cstddef>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// Maximum number of bytes in one read/write operation
constexpr std::string_view::size_type io_max = std::numeric_limits<int>::max();

/// A block size for file reading
constexpr std::size_t file_block_size = 4096;

/// Creates a temporary file with the given prefix in the system's
/// temporary directory, and opens it for reading and writing
/// @param label     A label to attach to the temporary file path
/// @param extension An extension of the temporary file path
/// @returns A path to the created temporary file and a handle to it
/// @throws fs::filesystem_error  if cannot create a temporary file
/// @throws std::invalid_argument if the label or extension is ill-formatted
std::pair<fs::path, entry::native_handle_type>
create_file(std::string_view label, std::string_view extension) {
  fs::path::string_type path = make_pattern(label, extension);

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary file", ec);
  }

#ifdef _WIN32
  HANDLE handle =
      CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    ec = std::error_code(GetLastError(), std::system_category());
  }
#else
  int handle = mkstemps(path.data(), static_cast<int>(extension.size()));
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create temporary file", ec);
  }

  return std::pair(path, handle);
}

/// Reads the content from the given native handle
/// @note uses handle current offset for reading
/// @param[in]  handle A native handle to read from
/// @param[out] ec     Parameter for error reporting
/// @returns A string with read contents
/// @throws std::bad_alloc if memory allocation fails
std::string read(entry::native_handle_type handle, std::error_code& ec) {
  std::ostringstream stream;

  std::array<std::string::value_type, file_block_size> buffer;

  while (true) {
    constexpr int readable = std::min(file_block_size, io_max);

#ifdef _WIN32
    DWORD read;
    if (!ReadFile(handle, buffer.data(), readable, &read, nullptr)) {
      ec = std::error_code(GetLastError(), std::system_category());
      return std::string();
    }
#else
    ssize_t read = ::read(handle, buffer.data(), readable);
    if (read < 0) {
      ec = std::error_code(errno, std::system_category());
      return std::string();
    }
#endif
    if (read == 0) {
      break;
    }

    stream.write(buffer.data(), read);
  }

  return std::move(stream).str();
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
}
}    // namespace

file::file(std::string_view label, std::string_view extension)
    : entry(create_file(label, extension)),
      binary(true) {}

file file::text(std::string_view label, std::string_view extension) {
  file result = file(label, extension);
  result.binary = false;

  return result;
}

file file::copy(const fs::path& path, std::string_view label,
                std::string_view extension) {
  file tmpfile = file(label, extension);

  std::error_code ec;
  fs::copy_file(path, tmpfile, copy_options, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
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
#ifdef _WIN32
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
  SetFilePointer(handle, 0, nullptr, FILE_BEGIN);
#else
  lseek(handle, 0, SEEK_SET);
#endif

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
  fs::resize_file(path(), 0, ec);
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
#ifdef _WIN32
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
  SetFilePointer(handle, 0, nullptr, FILE_END);
#else
  lseek(handle, 0, SEEK_END);
#endif

  tmp::write(handle, content, ec);
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
