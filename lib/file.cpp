#include <tmp/file.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

/// Creates a temporary file with the given prefix in the system's
/// temporary directory, and returns its path
/// @param prefix   The prefix to use for the temporary file name
/// @returns A path to the created temporary file
/// @throws fs::filesystem_error if the temporary file cannot be created
fs::path create(std::string_view prefix) {
    std::string pattern = tmp::path::make_pattern(prefix);
    if (mkstemp(pattern.data()) == -1) {
        std::error_code ec = std::error_code(errno, std::system_category());
        throw fs::filesystem_error("Cannot create temporary file", ec);
    }

    return pattern;
}

/// Opens a temporary file for writing and returns an output file stream
/// @param file     The file to open
/// @param binary   Whether to open the file in binary mode
/// @param append   Whether to append to the end of the file
/// @returns An output file stream
std::ofstream stream(const tmp::file& file, bool binary, bool append) noexcept {
    std::ios::openmode mode = append ? std::ios::app : std::ios::trunc;
    if (binary) {
        mode |= std::ios::binary;
    }

    const fs::path& path = file;
    return std::ofstream(path, mode);
}
}    // namespace

namespace tmp {

file::file(std::string_view prefix)
    : file(prefix, /*binary=*/true) {}

file::file(std::string_view prefix, bool binary)
    : path(create(prefix)),
      binary(binary) {}

file file::text(std::string_view prefix) {
    return file(prefix, /*binary=*/false);
}

void file::write(std::string_view content) const {
    stream(*this, binary, /*append=*/false) << content;
}

void file::append(std::string_view content) const {
    stream(*this, binary, /*append=*/true) << content;
}

file::~file() noexcept = default;

file::file(file&&) noexcept = default;
file& file::operator=(file&&) noexcept = default;
}    // namespace tmp
