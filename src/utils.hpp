#ifndef TMP_UTILS_H
#define TMP_UTILS_H

#include <filesystem>

namespace tmp {

namespace fs = std::filesystem;

/// Options for recursive overwriting copying
constexpr fs::copy_options copy_options =
    fs::copy_options::recursive | fs::copy_options::overwrite_existing;
}    // namespace tmp

#endif    // TMP_UTILS_H
