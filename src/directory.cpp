#include "abi.hpp"
#include "create.hpp"

#include <tmp/directory>

#include <filesystem>
#include <new>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {
namespace {

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;

/// Deletes a directory recursively, ignoring any errors
/// @param[in] path The path to the directory to delete
void remove_directory(const fs::path& path) noexcept {
  if (!path.empty()) {
    try {
      std::error_code ec;
      fs::remove_all(path, ec);
    } catch (const std::bad_alloc& ex) {
      static_cast<void>(ex);
    }
  }
}

/// Moves the directory recursively as if by `std::filesystem::rename`
/// even when moving between filesystems
/// @param[in]  from The path to the directory to move
/// @param[in]  to   The target path
/// @param[out] ec   Parameter for error reporting
void move_directory(const fs::path& from, const fs::path& to,
                    std::error_code& ec) {
  // FIXME: fs::is_directory can throw here
  // FIXME: Time-of-check to time-of-use
  if (fs::exists(to) && !fs::is_directory(to)) {
    ec = std::make_error_code(std::errc::not_a_directory);
    return;
  }

  bool copying = false;

#ifdef _WIN32
  // On Windows, the underlying `MoveFileExW` fails when moving a directory
  // between drives; in that case we copy the directory manually
  copying = from.root_name() != to.root_name();
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
    remove_directory(from);
  }
}
}    // namespace

directory::directory(std::string_view prefix)
    : pathobject(create_directory(prefix)) {}

directory directory::copy(const fs::path& path, std::string_view prefix) {
  directory tmpdir = directory(prefix);

  // We don't use `fs::copy(path, tmpdir)` here,
  // since there is no way to tell it to fail if `from` is not a directory;
  // a properly implemented `fs::directory_iterator` opens a path and checks
  // whether it is a directory atomically
  for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
    fs::copy(entry.path(), tmpdir / entry.path().filename(), copy_options);
  }

  return tmpdir;
}

directory::operator const fs::path&() const noexcept {
  return pathobject;
}

const fs::path& directory::path() const noexcept {
  return pathobject;
}

fs::path directory::operator/(const fs::path& source) const {
  return path() / source;
}

void directory::move(const fs::path& to) {
  std::error_code ec;
  move_directory(*this, to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary directory", path(), to,
                               ec);
  }

  pathobject.clear();
}

directory::~directory() noexcept {
  (void)reserved;    // Old compilers do not want to accept `[[maybe_unused]]`
  remove_directory(*this);
}

directory::directory(directory&& other) noexcept
    : pathobject(std::exchange(other.pathobject, fs::path())) {}

directory& directory::operator=(directory&& other) {
  // Here we intentionally call the throwing overload of `fs::remove_all`
  // to report errors with exceptions when deleting the old directory
  fs::remove_all(path());

  pathobject = std::exchange(other.pathobject, fs::path());
  return *this;
}
}    // namespace tmp
