#ifndef TMP_UTILS_H
#define TMP_UTILS_H

#include <tmp/entry>

#include <array>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#define UNICODE
#define NOMINMAX
#include <Windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {

namespace fs = std::filesystem;

/// Maximum number of bytes in one read/write operation
constexpr std::string_view::size_type io_max = std::numeric_limits<int>::max();

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;

/// Creates the parent directory of the given path if it does not exist
/// @param[in]  path The path for which the parent directory needs to be created
/// @param[out] ec   Parameter for error reporting
/// @returns @c true if a parent directory was newly created, @c false otherwise
/// @throws std::bad_alloc if memory allocation fails
bool create_parent(const fs::path& path, std::error_code& ec);

/// Creates a temporary path pattern with the given label and extension
/// @param[in] label     A label to attach to the path pattern
/// @param[in] extension An extension of the temporary file path
/// @returns A path pattern for the unique temporary path
/// @throws std::invalid_argument if the label or extension is ill-formatted
/// @throws std::bad_alloc        if memory allocation fails
fs::path make_pattern(std::string_view label, std::string_view extension);

/// Reads the content from the given native handle
/// @note uses handle current offset for reading
/// @tparam BlockSize  The read buffer size
/// @param[in]  handle A native handle to read from
/// @param[out] ec     Parameter for error reporting
/// @returns A string with read contents
/// @throws std::bad_alloc if memory allocation fails
template<std::size_t BlockSize>
std::string read(entry::native_handle_type handle, std::error_code& ec) {
  std::ostringstream stream;

  std::array<std::string::value_type, BlockSize> buffer;

  while (true) {
    constexpr int readable = static_cast<int>(std::min(BlockSize, io_max));

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
           std::error_code& ec) noexcept;
}    // namespace tmp

#endif    // TMP_UTILS_H
