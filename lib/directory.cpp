#include <tmp/directory.hpp>

#include <unistd.h>

namespace {

/// Creates a temporary directory with the given prefix in the system's
/// temporary directory, and returns its path
/// @param prefix   The prefix to use for the temporary file name
/// @returns A path to the created temporary file
/// @throws std::filesystem::filesystem_error if the temporary directory
///                                           cannot be created
std::filesystem::path create(std::string_view prefix) {
    auto pattern = tmp::path::make_pattern(prefix);
    if (mkdtemp(pattern.data()) == nullptr) {
        throw std::filesystem::filesystem_error(
            "Cannot create temporary directory",
            std::error_code(errno, std::system_category())
        );
    }

    return pattern;
}
}    // namespace

namespace tmp {

directory::directory(std::string_view prefix)
    : path(create(prefix)) {}

std::filesystem::path directory::operator/(std::string_view source) const {
    return static_cast<const std::filesystem::path&>(*this) / source;
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;
}    // namespace tmp
