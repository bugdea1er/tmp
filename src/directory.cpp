#include <tmp/directory>
#include <tmp/entry>

#include "create.hpp"
#include "move.hpp"

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

void directory::move(const fs::path& to) {
  std::error_code ec;
  move(to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary directory", path(), to,
                               ec);
  }
}

void directory::move(const fs::path& to, std::error_code& ec) {
  tmp::move(*this, to, ec);
  if (!ec) {
    clear();
  }
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;
}    // namespace tmp
