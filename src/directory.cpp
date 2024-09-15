#include <tmp/directory>
#include <tmp/entry>

#include "utils.hpp"

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory, and returns its path
/// @param label    A label to attach to the temporary directory path
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error  if cannot create a temporary directory
/// @throws std::invalid_argument if the label is ill-formatted
std::pair<fs::path, entry::native_handle_type>
create_directory(std::string_view label) {
  fs::path::string_type path = make_pattern(label, "");

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create temporary directory", ec);
  }

#ifdef _WIN32
  if (!CreateDirectoryW(path.c_str(), nullptr)) {
    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS) {
      return create_directory(label);
    }

    ec = std::error_code(err, std::system_category());
  }
#else
  if (mkdtemp(path.data()) == nullptr) {
    ec = std::error_code(errno, std::system_category());
  }

  int handle = open(path.data(), O_DIRECTORY);
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create temporary directory", ec);
  }

  return std::pair(path, handle);
}
}    // namespace

directory::directory(std::pair<fs::path, native_handle_type> handle) noexcept
    : entry(std::move(handle.first), handle.second) {}

directory::directory(std::string_view label)
    : directory(create_directory(label)) {}

directory directory::copy(const fs::path& path, std::string_view label) {
  std::error_code ec;
  directory tmpdir = directory(label);

  if (fs::is_directory(path)) {
    fs::copy(path, tmpdir, copy_options, ec);
  } else {
    ec = std::make_error_code(std::errc::not_a_directory);
  }

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpdir;
}

fs::path directory::operator/(std::string_view source) const {
  return path() / source;
}

fs::directory_iterator directory::list() const {
  return fs::directory_iterator(path());
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;
}    // namespace tmp

std::size_t std::hash<tmp::directory>::operator()(
    const tmp::directory& directory) const noexcept {
  return std::hash<tmp::entry>()(directory);
}
