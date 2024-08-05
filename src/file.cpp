#include <tmp/file>

#include "utils.hpp"

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

#ifdef WIN32
// Confirm that `HANDLE` is `void*` as implemented in `file`
static_assert(std::is_same_v<HANDLE, void*>);
#endif

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
  int handle = mkstemps(path.data(), static_cast<int>(suffix.size()));
  if (handle == -1) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create temporary file", ec);
  }

  return std::pair(path, handle);
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

/// Closes the given file, ignoring any errors
/// @param file     The file to close
void close(const file& file) noexcept {
#ifdef WIN32
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
  entry::operator=(std::move(other));

  close(*this);

  this->binary = other.binary;    // NOLINT(bugprone-use-after-move)
  this->handle = other.handle;    // NOLINT(bugprone-use-after-move)

  return *this;
}
}    // namespace tmp
