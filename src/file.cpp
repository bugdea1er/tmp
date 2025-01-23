#include <tmp/entry>
#include <tmp/file>

#include "create.hpp"

#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <utility>

namespace tmp {

file::file(std::pair<std::filesystem::path, std::fstream> handle) noexcept
    : entry(std::move(handle.first)),
      std::fstream(std::move(handle.second)) {}

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

void file::move(const std::filesystem::path& to) {
  entry::move(to);
}

void file::move(const std::filesystem::path& to, std::error_code& ec) {
  entry::move(to, ec);
}

file::~file() noexcept = default;

file::file(file&& other) noexcept    // FIXME: this doesn't look right at all
    : entry(std::move(static_cast<entry&&>(other))),
      std::fstream(std::move(static_cast<entry&&>(other))) {}
file& file::operator=(file&&) noexcept = default;
}    // namespace tmp
