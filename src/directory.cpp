#include <tmp/directory>
#include <tmp/entry>

#include "create.hpp"

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {
namespace {

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;
}    // namespace

directory::directory(std::string_view label)
    : entry(create_directory(label)) {}

directory::directory(std::error_code& ec)
    : entry(create_directory("", ec)) {}

directory::directory(std::string_view label, std::error_code& ec)
    : entry(create_directory(label, ec)) {}

directory directory::copy(const fs::path& path, std::string_view label) {
  directory tmpdir = directory(label);

  std::error_code ec;
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
  std::error_code ec;
  fs::directory_iterator iterator = list(ec);
  if (ec) {
    throw fs::filesystem_error("Cannot list a temporary directory", path(), ec);
  }

  return iterator;
}

fs::directory_iterator directory::list(std::error_code& ec) const {
  return fs::directory_iterator(path(), ec);
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;
}    // namespace tmp

std::size_t std::hash<tmp::directory>::operator()(
    const tmp::directory& directory) const noexcept {
  return std::hash<tmp::entry>()(directory);
}
