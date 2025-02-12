#include <tmp/directory>
#include <tmp/entry>

#include "create.hpp"
#include "move.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {

directory::directory(std::string_view label)
    : pathobject(create_directory(label)) {}

directory::operator const fs::path&() const noexcept {
  return pathobject;
}

const fs::path& directory::path() const noexcept {
  return *this;
}

fs::path directory::operator/(std::string_view source) const {
  return path() / source;
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
