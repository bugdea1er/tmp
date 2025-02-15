#include <tmp/directory>

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

directory::directory(std::string_view prefix)
    : pathobject(create_directory(prefix)) {}

directory directory::copy(const fs::path& path, std::string_view prefix) {
  directory tmpdir = directory(prefix);

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

directory::operator const fs::path&() const noexcept {
  return pathobject;
}

const fs::path& directory::path() const noexcept {
  return *this;
}

fs::path directory::operator/(std::string_view source) const {
  return path() / source;
}

void directory::move(const fs::path& to) {
  std::error_code ec;
  tmp::move(*this, to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary directory", path(), to,
                               ec);
  }

  pathobject.clear();
}

directory::~directory() noexcept {
  remove(*this);
}

directory::directory(directory&& other) noexcept
    : pathobject(std::move(other.pathobject)) {
  other.pathobject.clear();
}

directory& directory::operator=(directory&& other) noexcept {
  remove(*this);

  pathobject = std::move(other.pathobject);
  other.pathobject.clear();

  return *this;
}
}    // namespace tmp

std::size_t std::hash<tmp::directory>::operator()(
    const tmp::directory& directory) const noexcept {
  // `std::hash<std::filesystem::path>` was not included in the C++17 standard
  return filesystem::hash_value(directory);
}
