#include "abi.hpp"
#include "create.hpp"

#include <tmp/directory>

#include <filesystem>
#include <string_view>
#include <sys/fcntl.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace tmp {
namespace {

/// Deletes a directory recursively, ignoring any errors
/// @param[in] directory The directory to delete
void remove_directory(const directory& directory) noexcept {
  try {
    if (!directory.path().empty()) {
      // Calling the `std::error_code` overload of `fs::remove_all` should be
      // more optimal here since it would not require creating
      // a `fs::filesystem_error` message before we suppress the exception
      std::error_code ec;
      fs::remove_all(directory, ec);
    }
  } catch (...) {
    // Do nothing: if we failed to delete the temporary directory,
    // the system should do it later
  }
}
}    // namespace

directory::directory(std::string_view prefix)
    : pathobject(create_directory(prefix)),
      handle(open(pathobject.c_str(), O_DIRECTORY | O_SHLOCK)) {
  if (handle == -1) {
    // TODO: try again
    // TODO: does constructor delete path here?
    std::error_code ec = std::error_code(errno, std::system_category());
    throw fs::filesystem_error("Cannot create a temporary directory", ec);
  }
}

directory directory::copy(const fs::path& path, std::string_view prefix) {
  directory dir = directory(prefix);

  // We don't use `fs::copy(path, dir)` here,
  // since there is no way to tell it to fail if `from` is not a directory;
  // a properly implemented `fs::directory_iterator` opens a path and checks
  // whether it is a directory atomically

  std::error_code ec;
  for (fs::directory_iterator it = fs::directory_iterator(path, ec);
       !ec && it != fs::directory_iterator(); it.increment(ec)) {
    fs::copy(*it, dir / it->path().filename(), fs::copy_options::recursive, ec);
    if (ec) {
      break;
    }
  }

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
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
  close(handle);
  remove_directory(*this);
}

directory::directory(directory&& other) noexcept
    : pathobject(std::exchange(other.pathobject, fs::path())),
      handle(std::exchange(other.handle, -1)) {}

directory& directory::operator=(directory&& other) noexcept {
  close(handle);
  remove_directory(*this);

  pathobject = std::exchange(other.pathobject, fs::path());
  handle = std::exchange(other.handle, -1);
  return *this;
}
}    // namespace tmp
