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

file::file(std::pair<std::filesystem::path, std::filebuf> handle) noexcept
    : entry(std::move(handle.first)),
      std::iostream(&sb),
      sb(std::move(handle.second)) {}

file::file(std::string_view label, std::string_view extension, openmode mode)
    : file(create_file(label, extension, mode)) {}

file file::copy(const fs::path& path, std::string_view label,
                std::string_view extension, openmode mode) {
  file tmpfile = file(label, extension, mode);

  std::error_code ec;
  fs::copy_file(path, tmpfile, fs::copy_options::overwrite_existing, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

std::filebuf* file::rdbuf() const {
  // For `std::basic_fstream` C++ standard literally requires using `const_cast`
  return const_cast<std::filebuf*>(std::addressof(sb));    // NOLINT(*-cast)
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

file::~file() noexcept = default;

file::file(file&& other)
    : entry(std::move(other)),
      std::iostream(&sb),
      sb(std::move(other.sb)) {}    // NOLINT(*-use-after-move)

file& file::operator=(file&& other) {
  // The stream buffer must be assigned first to close the file
  // otherwise `entry` will not be able to remove the file before reassigning
  sb = std::move(other.sb);
  entry::operator=(std::move(other));

  return *this;
}
}    // namespace tmp
