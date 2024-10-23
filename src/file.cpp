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
#include <utility>

#ifdef _WIN32
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
      CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
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

file::~file() noexcept = default;

file::file(file&&) noexcept = default;
file& file::operator=(file&& other) noexcept = default;
}    // namespace tmp

std::size_t
std::hash<tmp::file>::operator()(const tmp::file& file) const noexcept {
  return std::hash<tmp::entry>()(file);
}
