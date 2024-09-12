#include <tmp/entry>

#include "utils.hpp"

#include <filesystem>
#include <new>
#include <system_error>
#include <utility>

namespace tmp {
namespace {

/// Deletes the given path recursively, ignoring any errors
/// @param path     The path to delete
void remove(const fs::path& path) noexcept {
  if (!path.empty()) {
    try {
      std::error_code ec;
      fs::remove_all(path, ec);

      fs::path parent = path.parent_path();
      if (!fs::equivalent(parent, fs::temp_directory_path(), ec)) {
        fs::remove(parent, ec);
      }
    } catch (const std::bad_alloc& ex) {
      static_cast<void>(ex);
    }
  }
}

/// Throws a filesystem error indicating that a temporary resource cannot be
/// moved to the specified path
/// @param to   The target path where the resource was intended to be moved
/// @param ec   The error code associated with the failure to move the resource
/// @throws fs::filesystem_error when called
[[noreturn]] void throw_move_error(const fs::path& to, std::error_code ec) {
  throw fs::filesystem_error("Cannot move temporary resource", to, ec);
}
}    // namespace

entry::entry(fs::path path)
    : pathobject(std::move(path)) {}

entry::entry(entry&& other) noexcept
    : pathobject(other.release()) {}

entry& entry::operator=(entry&& other) noexcept {
  remove(*this);
  pathobject = other.release();
  return *this;
}

entry::~entry() noexcept {
  remove(*this);
}

entry::operator const fs::path&() const noexcept {
  return pathobject;
}

const fs::path& entry::path() const noexcept {
  return *this;
}

void entry::move(const fs::path& to) {
  std::error_code ec;
  create_parent(to, ec);
  if (ec) {
    throw_move_error(to, ec);
  }

  fs::rename(*this, to, ec);
  if (ec == std::errc::cross_device_link) {
    if (fs::is_regular_file(*this) && fs::is_directory(to)) {
      ec = std::make_error_code(std::errc::is_a_directory);
      throw_move_error(to, ec);
    }

    fs::remove_all(to);
    fs::copy(*this, to, copy_options, ec);
  }

  if (ec) {
    throw_move_error(to, ec);
  }

  remove(*this);
  release();
}

fs::path entry::release() noexcept {
  fs::path path = std::move(pathobject);
  pathobject.clear();

  return path;
}
}    // namespace tmp
