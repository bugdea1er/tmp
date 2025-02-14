#include "move.hpp"

#include <filesystem>
#include <new>
#include <system_error>

namespace tmp {
namespace {

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;
}    // namespace

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
}    // namespace tmp
