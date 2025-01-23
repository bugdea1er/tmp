#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <string_view>
#include <system_error>
#include <type_traits>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace tmp {
namespace {

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<file::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is as implemented in `entry`
static_assert(std::is_same_v<HANDLE, file::native_handle_type>);
#endif

/// Implementation-defined invalid handle to the file
#ifdef _WIN32
const file::native_handle_type invalid_handle = INVALID_HANDLE_VALUE;
#else
constexpr file::native_handle_type invalid_handle = -1;
#endif

/// Closes the given entry, ignoring any errors
/// @param[in] file The file to close
void close(const file& file) noexcept {
#ifdef _WIN32
  CloseHandle(file.native_handle());
#else
  ::close(file.native_handle());
#endif
}
}    // namespace

file::file(std::pair<std::filesystem::path, file::native_handle_type> handle,
           bool binary) noexcept
    : entry(std::move(handle.first)),
      binary(binary),
      handle(handle.second) {}

file::file(std::string_view label, std::string_view extension)
    : file(create_file(label, extension), /*binary=*/true) {}

file::file(std::error_code& ec)
    : file(create_file("", "", ec), /*binary=*/true) {}

file::file(std::string_view label, std::error_code& ec)
    : file(create_file(label, "", ec), /*binary=*/true) {}

file::file(std::string_view label, std::string_view extension,
           std::error_code& ec)
    : file(create_file(label, extension, ec), /*binary=*/true) {}

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

file file::copy(const fs::path& path, std::string_view label,
                std::string_view extension) {
  file tmpfile = file(label, extension);

  std::error_code ec;
  fs::copy_file(path, tmpfile, fs::copy_options::overwrite_existing, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

file::native_handle_type file::native_handle() const noexcept {
  return handle;
}

std::ifstream file::input_stream() const {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();
  return std::ifstream(path(), mode);
}

std::ofstream file::output_stream(std::ios::openmode mode) const {
  binary ? mode |= std::ios::binary : mode &= ~std::ios::binary;
  return std::ofstream(path(), mode);
}

file::~file() noexcept {
  close(*this);
}

file::file(file&& other) noexcept
    : entry(std::move(other)),
      binary(other.binary),    // NOLINT(bugprone-use-after-move)
      handle(other.handle)     // NOLINT(bugprone-use-after-move)
{
  other.handle = invalid_handle;    // NOLINT(bugprone-use-after-move)
}

file& file::operator=(file&& other) noexcept {
  entry::operator=(std::move(other));
  close(*this);

  binary = other.binary;    // NOLINT(bugprone-use-after-move)
  handle = other.handle;    // NOLINT(bugprone-use-after-move)

  other.handle = invalid_handle;

  return *this;
}
}    // namespace tmp
