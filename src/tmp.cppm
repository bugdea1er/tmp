module;

#include <filesystem>
#include <tmp/directory>
#include <tmp/file>
#include <tmp/fs>
#include <tmp/path>

export module tmp;

export {
  namespace tmp {
  using tmp::directory;
  using tmp::file;
  using tmp::path;
  namespace fs = std::filesystem;
  } // namespace tmp
}
