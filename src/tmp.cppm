module;

#include <tmp/directory>
#include <tmp/file>
#include <tmp/filesystem>
#include <tmp/path>

export module tmp;

export {
    namespace tmp {
    using tmp::directory;
    using tmp::file;
    using tmp::filesystem;
    using tmp::path;
    }    // namespace tmp
}
