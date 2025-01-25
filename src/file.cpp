#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"
#include "move.hpp"

#include <filesystem>
#include <ios>
#include <istream>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {
namespace {

/// A mode to open a text file with
constexpr std::ios::openmode text_mode = std::ios::in | std::ios::out;

/// A mode to open a binary file with
constexpr std::ios::openmode binary_mode = std::ios::binary | text_mode;
}    // namespace

file::file(std::pair<std::filesystem::path, std::filebuf> handle) noexcept
    : entry(std::move(handle.first)),
      std::iostream(&filebuf),
      filebuf(std::move(handle.second)) {}

file::file(std::string_view label, std::string_view extension)
    : file(create_file(label, extension, binary_mode)) {}

file file::text(std::string_view label, std::string_view extension) {
  return file(create_file(label, extension, text_mode));
}

file file::copy(const fs::path& path, std::string_view label,
                std::string_view extension) {
  file tmpfile = file(label, extension);

  std::error_code ec;
  fs::copy_file(path, tmpfile, fs::copy_options::overwrite_existing, ec);

  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary copy", path, ec);
  }

  return tmpfile;
}

void file::move(const fs::path& to) {
  filebuf.close();

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
      std::iostream(&filebuf),
      filebuf(std::move(other.filebuf)) {}    // NOLINT(*-use-after-move)

file& file::operator=(file&& other) {
  // `filebuf` must be assigned first to close the file
  // otherwise `entry` will not be able to remove the file before reassigning
  filebuf = std::move(other.filebuf);
  entry::operator=(std::move(other));

  return *this;
}
}    // namespace tmp
