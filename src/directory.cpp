#include <tmp/directory>
#include <tmp/entry>

#include "create.hpp"
#include "move.hpp"

#include <filesystem>
#include <string_view>
#include <system_error>

namespace tmp {

directory::directory(std::string_view label)
    : entry(create_directory(label)) {}

fs::path directory::operator/(std::string_view source) const {
  return path() / source;
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;
}    // namespace tmp
