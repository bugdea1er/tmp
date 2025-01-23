#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <utility>

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Tests directory creation with label
TEST(directory, create_with_label) {
  directory tmpdir = directory(LABEL);
  fs::path parent = tmpdir.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::is_directory(tmpdir));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / LABEL));

  fs::perms permissions = fs::status(tmpdir).permissions();
#ifdef _WIN32
  // Following the logic with GetTempFileNameW
  EXPECT_EQ(permissions, fs::perms::all);
#else
  // mkdtemp creates a directory with full access only for the owner
  EXPECT_EQ(permissions, fs::perms::owner_all);
#endif
}

/// Tests directory creation without label
TEST(directory, create_without_label) {
  directory tmpdir = directory();
  fs::path parent = tmpdir.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::is_directory(tmpdir));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
}

/// Tests multiple directories creation with the same label
TEST(directory, create_multiple) {
  directory fst = directory(LABEL);
  directory snd = directory(LABEL);

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests error handling with invalid labels
TEST(directory, create_invalid_label) {
  EXPECT_THROW(directory("multi/segment"), std::invalid_argument);
  EXPECT_THROW(directory("/root"), std::invalid_argument);
  EXPECT_THROW(directory(".."), std::invalid_argument);
  EXPECT_THROW(directory("."), std::invalid_argument);

  fs::path root = fs::temp_directory_path().root_name();
  if (!root.empty()) {
    EXPECT_THROW(directory(root.string() + "relative"), std::invalid_argument);
    EXPECT_THROW(directory(root.string() + "/root"), std::invalid_argument);
  }
}

/// Tests creation of a temporary copy of a directory
TEST(directory, copy_directory) {
  directory tmpdir = directory();
  std::ofstream(tmpdir / "file") << "Hello, world!";

  directory copy = directory::copy(tmpdir);
  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::exists(copy));
  EXPECT_FALSE(fs::equivalent(tmpdir, copy));

  EXPECT_TRUE(fs::is_directory(copy));

  std::ifstream stream = std::ifstream(copy / "file");
  auto content = std::string(std::istreambuf_iterator<char>(stream), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of a temporary copy of a file
TEST(directory, copy_file) {
  file tmpfile = file();
  EXPECT_THROW(directory::copy(tmpfile), fs::filesystem_error);
}

/// Tests `operator/` of directory
TEST(directory, subpath) {
  directory tmpdir = directory();
  fs::path child = tmpdir / "child";

  EXPECT_TRUE(fs::equivalent(tmpdir, child.parent_path()));
}

/// Tests directory listing
TEST(directory, list) {
  directory tmpdir = directory();
  std::ofstream(tmpdir / "file1") << "Hello, world!";
  std::ofstream(tmpdir / "file2") << "Hello, world!";

  fs::create_directory(tmpdir / "subdir");
  std::ofstream(tmpdir / "subdir" / "file") << "Hello, world!";

  std::set entries = std::set<fs::path>();
  for (const auto& entry : tmpdir.list()) {
    entries.insert(fs::relative(entry, tmpdir));
  }

  EXPECT_EQ(entries, std::set<fs::path>({"file1", "file2", "subdir"}));
}

/// Tests that destructor removes a directory
TEST(directory, destructor) {
  fs::path path;
  {
    directory tmpdir = directory();
    path = tmpdir;
  }

  EXPECT_FALSE(fs::exists(path));
}

/// Tests directory move constructor
TEST(directory, move_constructor) {
  directory fst = directory();
  directory snd = directory(std::move(fst));

  fst.~directory();    // NOLINT(*-use-after-move)

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
}

/// Tests directory move assignment operator
TEST(directory, move_assignment) {
  directory fst = directory();
  {
    directory snd = directory();

    fs::path path1 = fst;
    fs::path path2 = snd;

    fst = std::move(snd);

    EXPECT_FALSE(fs::exists(path1));
    EXPECT_TRUE(fs::exists(path2));

    EXPECT_TRUE(fs::exists(fst));
    EXPECT_TRUE(fs::equivalent(fst, path2));
  }

  EXPECT_FALSE(fst.path().empty());
}

/// Tests directory swapping
TEST(directory, swap) {
  directory fst = directory();
  directory snd = directory();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
}

/// Tests directory hashing
TEST(directory, hash) {
  directory tmpdir = directory();
  std::hash hash = std::hash<directory>();

  EXPECT_EQ(hash(tmpdir), fs::hash_value(tmpdir.path()));
}

/// Tests directory relational operators
TEST(directory, relational) {
  directory tmpdir = directory();

  EXPECT_TRUE(tmpdir == tmpdir);
  EXPECT_FALSE(tmpdir < tmpdir);
}
}    // namespace
}    // namespace tmp
