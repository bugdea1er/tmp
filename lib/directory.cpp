#include <tmp/directory.hpp>

namespace tmp {

directory::directory(std::string_view prefix) : path(create(prefix)) {}

std::filesystem::path directory::operator/(std::string_view source) const {
    return static_cast<const std::filesystem::path&>(*this) / source;
}

directory::~directory() noexcept = default;

directory::directory(directory&&) noexcept = default;
directory& directory::operator=(directory&&) noexcept = default;

std::filesystem::path directory::create(std::string_view prefix) {
    auto pattern = make_pattern(prefix);
    if (mkdtemp(pattern.data()) == nullptr) {
        auto ec = std::error_code(errno, std::system_category());
        throw error("Cannot create temporary directory", ec);
    }

    return pattern;
}

}    // namespace tmp
