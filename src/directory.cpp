#include <tmp/directory>

#include "utils.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>

#ifdef WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <unistd.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory, and returns its path
/// @param prefix   The prefix to use for the temporary directory name
/// @returns A path to the created temporary directory
/// @throws fs::filesystem_error if cannot create the temporary directory
fs::path create_directory(std::string_view prefix) {
  fs::path::string_type path = make_pattern(prefix, "");

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create temporary directory", ec);
  }

#ifdef WIN32
  if (!CreateDirectoryW(path.c_str(), nullptr)) {
    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS) {
      return create_directory(prefix);
    }

    ec = std::error_code(err, std::system_category());
  }
#else
  if (mkdtemp(path.data()) == nullptr) {
    ec = std::error_code(errno, std::system_category());
  }
#endif

  if (ec) {
    throw fs::filesystem_error("Cannot create temporary directory", ec);
  }

  return path;
}
}    // namespace

directory::directory(std::string_view prefix)
    : entry(create_directory(prefix)) {}

directory directory::copy(const fs::path& path, std::string_view prefix) {
  std::error_code ec;
  directory tmpdir = directory(prefix);

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
