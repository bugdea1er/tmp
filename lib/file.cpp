#include <tmp/file.hpp>

#include <cstdlib>
#include <fstream>

namespace tmp {

namespace {

/// Opens a temporary file for writing and returns an output file stream
/// @param file     The file to open
/// @param binary   Whether to open the file in binary mode
/// @param append   Whether to append to the end of the file
/// @return An output file stream
std::ofstream stream(const file& file, bool binary, bool append) noexcept {
    std::ios::openmode mode = append ? std::ios::app : std::ios::trunc;
    if (binary) {
        mode |= std::ios::binary;
    }

    const std::filesystem::path& path = file;
    return std::ofstream { path, mode };
}
}    // namespace

file::file(std::string_view prefix) : file(prefix, /*binary=*/true) {
}

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

file::file(std::string_view prefix, bool binary) : path(create(prefix)),
                                                   binary(binary) {
}

std::filesystem::path file::create(std::string_view prefix) {
    auto pattern = make_pattern(prefix);
    if (mkstemp(pattern.data()) == -1) {
        throw std::filesystem::filesystem_error(
            "Cannot create temporary file",
            std::error_code(errno, std::system_category())
        );
    }

    return pattern;
}

}    // namespace tmp
