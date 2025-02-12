#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"
#include "move.hpp"

#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {

file::file(std::ios::openmode mode)
    : std::iostream(std::addressof(sb)) {
  if (sb.open(mode) == nullptr) {
    std::error_code ec = std::make_error_code(std::io_errc::stream);
    throw std::filesystem::filesystem_error("", ec);
  }
}

filebuf* file::rdbuf() const noexcept {
  return const_cast<filebuf*>(std::addressof(sb));    // NOLINT(*-const-cast)
}

file::~file() noexcept {
  sb.close();
}

// NOLINTBEGIN(*-use-after-move)
file::file(file&& other) noexcept
    : std::iostream(std::move(other)),
      sb(std::move(other.sb)) {
  set_rdbuf(std::addressof(sb));
}

file& file::operator=(file&& other) {
  std::iostream::operator=(std::move(other));
  sb = std::move(other.sb);

  return *this;
}
// NOLINTEND(*-use-after-move)
}    // namespace tmp
