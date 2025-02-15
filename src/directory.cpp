#include <tmp/directory>

#include "create.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {
namespace {

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;

/// Deletes the given path recursively, ignoring any errors
/// @param[in] path The path to delete
void remove(const fs::path& path) noexcept {
  if (!path.empty()) {
    try {
      std::error_code ec;
      fs::remove_all(path, ec);
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
  // FIXME: `fs::is_directory can fail here`
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
