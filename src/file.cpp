#include <tmp/entry>
#include <tmp/file>

#include "utils.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {
namespace {

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<file::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is `void*` as implemented in `file`
static_assert(std::is_same_v<HANDLE, void*>);
#endif

/// Creates a temporary file with the given prefix in the system's
/// temporary directory, and opens it for reading and writing
///
/// @param label     A label to attach to the temporary file path
/// @param extension An extension of the temporary file path
/// @returns A path to the created temporary file and a handle to it
/// @throws fs::filesystem_error  if cannot create a temporary file
/// @throws std::invalid_argument if the label or extension is ill-formatted
std::pair<fs::path, file::native_handle_type>
create_file(std::string_view label, std::string_view extension) {
  fs::path::string_type path = make_pattern(label, extension);

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create temporary file", ec);
  }

#ifdef _WIN32
  HANDLE handle =
      CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS) {
      return create_file(label, extension);
    }

    ec = std::error_code(err, std::system_category());
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

/// Closes the given file, ignoring any errors
/// @param file     The file to close
void close(const file& file) noexcept {
#ifdef _WIN32
  CloseHandle(file.native_handle());
#else
  ::close(file.native_handle());
#endif
}
}    // namespace

file::file(std::pair<fs::path, native_handle_type> handle, bool binary) noexcept
    : entry(std::move(handle.first)),
      handle(handle.second),
      binary(binary) {}

file::file(std::string_view label, std::string_view extension, bool binary)
    : file(create_file(label, extension), binary) {}

file::file(std::string_view label, std::string_view extension)
    : file(label, extension, /*binary=*/true) {}

file file::text(std::string_view label, std::string_view extension) {
  return file(label, extension, /*binary=*/false);
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

file::native_handle_type file::native_handle() const noexcept {
  return handle;
}

std::string file::read() const {
  std::ifstream stream = input_stream();
  return std::string(std::istreambuf_iterator<char>(stream), {});
}

void file::write(std::string_view content) const {
  output_stream(std::ios::trunc) << content;
}

void file::append(std::string_view content) const {
  output_stream(std::ios::app) << content;
}

std::ifstream file::input_stream() const {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode();
  return std::ifstream(path(), mode);
}

std::ofstream file::output_stream(std::ios::openmode mode) const {
  binary ? mode |= std::ios::binary : mode ^= std::ios::binary;
  return std::ofstream(path(), mode);
}

file::~file() noexcept {
  close(*this);
}

file::file(file&&) noexcept = default;

file& file::operator=(file&& other) noexcept {
  entry::operator=(std::move(other));

  close(*this);

  binary = other.binary;    // NOLINT(bugprone-use-after-move)
  handle = other.handle;    // NOLINT(bugprone-use-after-move)

  return *this;
}
}    // namespace tmp

std::size_t
std::hash<tmp::file>::operator()(const tmp::file& file) const noexcept {
  return std::hash<tmp::entry>()(file);
}
