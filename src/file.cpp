#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"

#include <filesystem>
#include <istream>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {

file::file(std::pair<std::filesystem::path, std::filebuf> handle) noexcept
    : entry(std::move(handle.first)),
      std::iostream(&filebuf),
      filebuf(std::move(handle.second)) {}

file::file(std::string_view label, std::string_view extension)
    : file(create_file(label, extension, /*binary=*/true)) {}

file::file(std::error_code& ec)
    : file(create_file("", "", /*binary=*/true, ec)) {}

file::file(std::string_view label, std::error_code& ec)
    : file(create_file(label, "", /*binary=*/true, ec)) {}

file::file(std::string_view label, std::string_view extension,
           std::error_code& ec)
    : file(create_file(label, extension, /*binary=*/true, ec)) {}

file file::text(std::string_view label, std::string_view extension) {
  return file(create_file(label, extension, /*binary=*/false));
}

file file::text(std::error_code& ec) {
  return text("", ec);
}

file file::text(std::string_view label, std::error_code& ec) {
  return text(label, "", ec);
}

file file::text(std::string_view label, std::string_view extension,
                std::error_code& ec) {
  return file(create_file(label, extension, /*binary=*/false, ec));
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

file::~file() noexcept = default;

// Cannot be defaulted since we need to reset rdbuf for iostream
// NOLINTBEGIN(*-use-after-move)
file::file(file&& other)
    : entry(std::move(other)),
      std::iostream(&filebuf),
      filebuf(std::move(other.filebuf)) {
}
// NOLINTEND(*-use-after-move)

file& file::operator=(file&& other) {
  // `filebuf` must be assigned first to close the file
  // otherwise `entry` will not be able to remove the file before reassigning
  filebuf = std::move(other.filebuf);
  entry::operator=(std::move(other));

  return *this;
}
}    // namespace tmp
