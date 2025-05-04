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
}    // namespace

directory::directory(std::string_view prefix)
    : pathobject(create_directory(prefix)) {}

directory directory::copy(const fs::path& path, std::string_view prefix) {
  directory dir = directory(prefix);

  // We don't use `fs::copy(path, dir)` here,
  // since there is no way to tell it to fail if `from` is not a directory;
  // a properly implemented `fs::directory_iterator` opens a path and checks
  // whether it is a directory atomically
  for (const fs::directory_entry& entry : fs::directory_iterator(path)) {
    fs::copy(entry, dir / entry.path().filename(), fs::copy_options::recursive);
  }

  return dir;
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
