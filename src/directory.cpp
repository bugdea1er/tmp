#include "abi.hpp"
#include "create.hpp"

#include <tmp/directory>

#include <filesystem>
#include <string_view>
#include <system_error>
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
    : pathobject(create_directory(prefix)) {}

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
  (void)handle;    // Old compilers do not want to accept `[[maybe_unused]]`
  remove_directory(*this);
}

directory::directory(directory&& other) noexcept
    : pathobject(std::exchange(other.pathobject, fs::path())) {}

directory& directory::operator=(directory&& other) noexcept {
  remove_directory(*this);

  pathobject = std::exchange(other.pathobject, fs::path());
  return *this;
}
}    // namespace tmp
