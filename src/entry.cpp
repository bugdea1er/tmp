#include <tmp/entry>

#include "utils.hpp"

#include <cstddef>
#include <filesystem>
#include <new>
#include <system_error>
#include <utility>

namespace tmp {
namespace {

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<entry::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is `void*` as implemented in `entry`
static_assert(std::is_same_v<HANDLE, void*>);
#endif

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

/// Throws a filesystem error indicating that a temporary entry cannot be
/// moved to the specified path
/// @param to       The target path where the entry was intended to be moved
/// @param ec       The error code associated with the failure to move the entry
/// @throws fs::filesystem_error when called
[[noreturn]] void throw_move_error(const fs::path& to, std::error_code ec) {
  throw fs::filesystem_error("Cannot move temporary entry", to, ec);
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

bool operator==(const entry& lhs, const entry& rhs) noexcept {
  return lhs.path() == rhs.path();
}

bool operator!=(const entry& lhs, const entry& rhs) noexcept {
  return lhs.path() != rhs.path();
}

bool operator<(const entry& lhs, const entry& rhs) noexcept {
  return lhs.path() < rhs.path();
}

bool operator<=(const entry& lhs, const entry& rhs) noexcept {
  return lhs.path() <= rhs.path();
}

bool operator>(const entry& lhs, const entry& rhs) noexcept {
  return lhs.path() > rhs.path();
}

bool operator>=(const entry& lhs, const entry& rhs) noexcept {
  return lhs.path() >= rhs.path();
}
}    // namespace tmp

std::size_t
std::hash<tmp::entry>::operator()(const tmp::entry& entry) const noexcept {
  return tmp::fs::hash_value(entry);
}
