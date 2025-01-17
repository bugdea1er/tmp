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
const entry::native_handle_type invalid_handle = INVALID_HANDLE_VALUE;
#else
constexpr entry::native_handle_type invalid_handle = -1;
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
/// @param path      The path to delete
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

/// Moves the filesystem object as if by `std::filesystem::rename`
/// even when moving between filesystems
/// @param[in]  from The path to move
/// @param[in]  to   A path to the target file or directory
/// @param[out] ec   Parameter for error reporting
/// @throws std::bad_alloc if memory allocation fails
void move(const fs::path& from, const fs::path& to, std::error_code& ec) {
  if (fs::exists(to)) {
    if (!fs::is_directory(from) && fs::is_directory(to)) {
      ec = std::make_error_code(std::errc::is_a_directory);
      return;
    }

    if (fs::is_directory(from) && !fs::is_directory(to)) {
      ec = std::make_error_code(std::errc::not_a_directory);
      return;
    }
  }

  create_parent(to, ec);
  if (ec) {
    return;
  }

  bool copying = false;

#ifdef _WIN32
  // On Windows, the underlying `MoveFileExW` fails when moving a directory
  // between drives; in that case we copy the directory manually
  copying = fs::is_directory(from) && from.root_name() != to.root_name();
  if (copying) {
    fs::copy(from, to, copy_options, ec);
  } else {
    fs::rename(from, to, ec);
  }
#else
  // On POSIX-compliant systems, the underlying `rename` function may return
  // `EXDEV` if the implementation does not support links between file systems;
  // so we try to rename the file, and if we fail with `EXDEV`, move it manually
  fs::rename(from, to, ec);
  copying = ec == std::errc::cross_device_link;
  if (copying) {
    fs::remove_all(to);
    fs::copy(from, to, copy_options, ec);
  }
#endif

  if (!ec && copying) {
    tmp::remove(from);
  }
}
}    // namespace

entry::entry(std::pair<std::filesystem::path, native_handle_type> handle)
    : pathobject(std::move(handle.first)),
      handle(handle.second) {}

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
  move(to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary entry", path(), to, ec);
  }
}

void entry::move(const fs::path& to, std::error_code& ec) {
  tmp::move(*this, to, ec);
  if (!ec) {
    pathobject.clear();
  }
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
