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

file::file(std::pair<fs::path, filebuf> handle) noexcept
    : entry(std::move(handle.first)),
      std::iostream(std::addressof(sb)),
      sb(std::move(handle.second)) {}

file::file(std::ios::openmode mode)
    : file(create_file(mode)) {}

file file::copy(const fs::path& path, std::ios::openmode mode) {
  file tmpfile = file(mode);

  std::error_code ec;
  fs::copy_file(path, tmpfile, fs::copy_options::overwrite_existing, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

file::native_handle_type file::native_handle() const noexcept {
  return sb.native_handle();
}

std::filebuf* file::rdbuf() const noexcept {
  return const_cast<filebuf*>(std::addressof(sb));    // NOLINT(*-const-cast)
}

void file::move(const fs::path& to) {
  sb.close();

  std::error_code ec;
  tmp::move(*this, to, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot move a temporary file", path(), to, ec);
  }

  entry::clear();
}

file::~file() noexcept {
  sb.close();
}

// NOLINTBEGIN(*-use-after-move)
file::file(file&& other) noexcept
    : entry(std::move(other)),
      std::iostream(std::move(other)),
      sb(std::move(other.sb)) {
  set_rdbuf(std::addressof(sb));
}

file& file::operator=(file&& other) {
  std::iostream::operator=(std::move(other));

  // The stream buffer must be assigned first to close the file;
  // otherwise `entry` may not be able to remove the file before reassigning
  sb = std::move(other.sb);
  entry::operator=(std::move(other));

  return *this;
}
// NOLINTEND(*-use-after-move)
}    // namespace tmp
