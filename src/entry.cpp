#include <tmp/entry>

#include "utils.hpp"

#include <cstddef>
#include <filesystem>
#include <new>
#include <system_error>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace tmp {
namespace {

// Confirm that native_handle_type matches `TriviallyCopyable` named requirement
static_assert(std::is_trivially_copyable_v<entry::native_handle_type>);

#ifdef _WIN32
// Confirm that `HANDLE` is `void*` as implemented in `entry`
static_assert(std::is_same_v<HANDLE, void*>);
#endif

/// Implementation-defined invalid handle to the entry
#ifdef _WIN32
const entry::native_handle_type invalid_handle = nullptr;
#else
const entry::native_handle_type invalid_handle = -1;
#endif

/// Closes the given entry, ignoring any errors
/// @param entry     The entry to close
void close(const entry& entry) noexcept {
#ifdef _WIN32
  CloseHandle(entry.native_handle());
#else
  ::close(entry.native_handle());
#endif
}

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

entry::entry(fs::path path, native_handle_type handle)
    : pathobject(std::move(path)),
      handle(handle) {}

entry::entry(std::pair<std::filesystem::path, native_handle_type> handle)
    : entry(std::move(handle.first), handle.second) {}

entry::entry(entry&& other) noexcept
    : pathobject(std::move(other.pathobject)),
      handle(other.handle) {
  other.pathobject.clear();
  other.handle = invalid_handle;
}

entry& entry::operator=(entry&& other) noexcept {
  close(*this);
  remove(*this);

  pathobject = std::move(other.pathobject);
  other.pathobject.clear();

  handle = other.handle;
  other.handle = invalid_handle;

  return *this;
}

entry::~entry() noexcept {
  close(*this);
  remove(*this);
}

entry::operator const fs::path&() const noexcept {
  return pathobject;
}

const fs::path& entry::path() const noexcept {
  return *this;
}

entry::native_handle_type entry::native_handle() const noexcept {
  return handle;
}

void entry::move(const fs::path& to) {
  std::error_code ec;
  if (fs::exists(to)) {
    if (!fs::is_directory(*this) && fs::is_directory(to)) {
      ec = std::make_error_code(std::errc::is_a_directory);
      throw_move_error(to, ec);
    }

    if (fs::is_directory(*this)) {
      if (!fs::is_directory(to)) {
        ec = std::make_error_code(std::errc::not_a_directory);
        throw_move_error(to, ec);
      }

#ifdef WIN32
      if (!fs::equivalent(path(), to)) {
        fs::remove_all(to);
      }
#endif
    }
  }

  create_parent(to, ec);
  if (ec) {
    throw_move_error(to, ec);
  }

  fs::rename(*this, to, ec);
  if (ec == std::errc::cross_device_link || ec == std::errc::permission_denied) {
    fs::remove_all(to);
    fs::copy(*this, to, copy_options, ec);
    remove(*this);
  }

  if (ec) {
    throw_move_error(to, ec);
  }

  pathobject.clear();
}

bool entry::operator==(const entry& rhs) const noexcept {
  return path() == rhs.path();
}

bool entry::operator!=(const entry& rhs) const noexcept {
  return path() != rhs.path();
}

bool entry::operator<(const entry& rhs) const noexcept {
  return path() < rhs.path();
}

bool entry::operator<=(const entry& rhs) const noexcept {
  return path() <= rhs.path();
}

bool entry::operator>(const entry& rhs) const noexcept {
  return path() > rhs.path();
}

bool entry::operator>=(const entry& rhs) const noexcept {
  return path() >= rhs.path();
}
}    // namespace tmp

std::size_t
std::hash<tmp::entry>::operator()(const tmp::entry& entry) const noexcept {
  return tmp::fs::hash_value(entry);
}
