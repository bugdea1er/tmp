#include <tmp/entry>

#include "create.hpp"
#include "move.hpp"

#include <cstddef>
#include <filesystem>

namespace tmp {

entry::entry(fs::path path) noexcept
    : pathobject(std::move(path)) {}

entry::entry(entry&& other) noexcept
    : pathobject(std::move(other.pathobject)) {
  other.clear();
}

entry& entry::operator=(entry&& other) noexcept {
  remove(*this);

  pathobject = std::move(other.pathobject);
  other.clear();

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

void entry::clear() noexcept {
  pathobject.clear();
}
}    // namespace tmp

std::size_t
std::hash<tmp::entry>::operator()(const tmp::entry& entry) const noexcept {
  // `std::hash<std::filesystem::path>` was not included in the C++17 standard
  return filesystem::hash_value(entry);
}
