#include <tmp/entry>
#include <tmp/file>

#include "utils.hpp"

#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string_view>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define UNICODE
#include <corecrt_io.h>
#include <Windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {
namespace {

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
    throw fs::filesystem_error("Cannot create temporary file", ec);
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
}    // namespace

file::file(std::string_view label, std::string_view extension)
    : entry(create_file(label, extension)),
      binary(/*binary=*/true) {}

file file::text(std::string_view label, std::string_view extension) {
  // A bit janky, but all in the name of the cleanest headers
  file result = file(label, extension);
  result.binary = false;

  return result;
}

file file::copy(const fs::path& path, std::string_view label,
                std::string_view extension) {
  std::error_code ec;
  file tmpfile = file(label, extension);

  fs::copy_file(path, tmpfile, copy_options, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

std::string file::read() const {
#ifdef _WIN32
  // On Windows, the native API does not distinguish between text and binary
  // files; opening a separate CRT file descriptor is the easiest way here
  int mode = _O_RDONLY | (binary ? _O_BINARY : _O_TEXT);

  int handle;
  std::ignore = _wsopen_s(&handle, path().c_str(), mode, _SH_DENYNO, _S_IREAD);
  auto closer = std::shared_ptr<nullptr_t>(nullptr, [&](auto) { _close(handle); });
#else
  // On POSIX-compliant systems, we can just use already open handle
  native_handle_type handle = native_handle();
  lseek(handle, 0, SEEK_SET);
#endif

  std::string content;
  content.resize(file_size(path()));

  std::size_t offset = 0;
  while (offset < content.size()) {
    // FIXME: handle large buffers
#ifdef _WIN32
    int bytes_read = _read(handle, &content[offset], content.size() - offset);
#else
    ssize_t bytes_read = ::read(handle, &content[offset], content.size() - offset);
#endif

    if (bytes_read == 0) {
      break;
    }

    if (bytes_read < 0) {
      std::error_code ec = std::error_code(errno, std::system_category());
      throw fs::filesystem_error("Cannot read a temporary file", path(), ec);
    }

    offset += bytes_read;
  }

  content.resize(offset);
  return content;
}

void file::write(std::string_view content) const {
  fs::resize_file(path(), 0);
  append(content);
}

void file::append(std::string_view content) const {
#ifdef _WIN32
  // On Windows, the native API does not distinguish between text and binary
  // files; opening a separate CRT file descriptor is the easiest way here
  int mode = _O_WRONLY | _O_APPEND | (binary ? _O_BINARY : _O_TEXT);

  int handle;
  std::ignore = _wsopen_s(&handle, path().c_str(), mode, _SH_DENYNO, _S_IWRITE);
  auto closer = std::shared_ptr<nullptr_t>(nullptr, [&](nullptr_t) { _close(handle); });
#else
  // On POSIX-compliant systems, we can just use already open handle
  native_handle_type handle = native_handle();
  lseek(handle, 0, SEEK_END);
#endif

  while (!content.empty()) {
    // `write` will fail if the parameter `nbyte` exceeds `INT_MAX`
    int nbyte = static_cast<int>(std::min<size_t>(content.size(), INT_MAX));

    auto bytes_written = ::write(handle, content.data(), nbyte);
    if (bytes_written < 0) {
      std::error_code ec = std::error_code(errno, std::system_category());
      throw fs::filesystem_error("Cannot write to a temporary file", path(), ec);
    }

    content = content.substr(bytes_written);
  }
}

std::ifstream file::input_stream() const {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();
  return std::ifstream(path(), mode);
}

std::ofstream file::output_stream(std::ios::openmode mode) const {
  binary ? mode |= std::ios::binary : mode ^= std::ios::binary;
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
