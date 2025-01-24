#include <tmp/entry>
#include <tmp/file>

#include <cstddef>
#include <filesystem>
#include <new>
#include <system_error>
#include <utility>

namespace tmp {
namespace {

namespace fs = std::filesystem;

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

entry::entry(fs::path path) noexcept
    : pathobject(std::move(path)) {}

entry::entry(entry&& other) noexcept
    : pathobject(std::move(other.pathobject)) {
  other.pathobject.clear();
}

entry& entry::operator=(entry&& other) noexcept {
  remove(*this);

  pathobject = std::move(other.pathobject);
  other.pathobject.clear();

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
  move(to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary entry", path(), to, ec);
  }
}

void entry::move(const fs::path& to, std::error_code& ec) {
  // FIXME: there must be a better way
#ifdef _WIN32
  file* tmpfile = dynamic_cast<file*>(this);
  if (tmpfile != nullptr) {
    std::filebuf* buf = dynamic_cast<std::filebuf*>(tmpfile->rdbuf());
    if (buf != nullptr) {
      buf->close();
    }
  }
#endif

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
  // `std::hash<std::filesystem::path>` was not included in the C++17 standard
  return filesystem::hash_value(entry);
}
