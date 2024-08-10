#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>

namespace tmp {

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

/// Tests creation of a temporary copy of a directory
TEST(directory, copy_directory) {
  directory tmpdir = directory();
  std::ofstream(tmpdir / "file") << "Hello, world!";

  directory tmpcopy = directory::copy(tmpdir);
  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::exists(tmpcopy));
  EXPECT_FALSE(fs::equivalent(tmpdir, tmpcopy));

  EXPECT_TRUE(fs::is_directory(tmpcopy));

  std::ifstream stream = std::ifstream(tmpcopy / "file");
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

  std::set<fs::path> entries;
  for (const auto& entry : tmpdir.list()) {
    entries.insert(fs::relative(entry, tmpdir));
  }

  EXPECT_EQ(entries, std::set<fs::path>({"file1", "file2", "subdir"}));
}

/// Tests that destructor removes a directory
TEST(directory, destructor) {
  fs::path path = fs::path();
  {
    directory tmpdir = directory();
    path = tmpdir;
  }

  EXPECT_FALSE(fs::exists(path));
}

/// Tests directory move constructor
TEST(directory, move_constructor) {
  directory fst = directory();
  directory snd = std::move(fst);

  EXPECT_TRUE(fst.path().empty());
  EXPECT_TRUE(fs::exists(snd));
}

/// Tests directory move assignment operator
TEST(directory, move_assignment) {
  directory fst = directory();
  directory snd = directory();

  fs::path path1 = fst;
  fs::path path2 = snd;

  fst = std::move(snd);

  EXPECT_FALSE(fs::exists(path1));
  EXPECT_TRUE(fs::exists(path2));

  EXPECT_TRUE(fs::exists(fst));
  EXPECT_TRUE(fs::equivalent(fst, path2));
}

/// Tests directory moving
TEST(directory, move) {
  fs::path path = fs::path();
  fs::path to = fs::temp_directory_path() / "non-existing" / "parent";
  {
    directory tmpdir = directory();
    path = tmpdir;

    tmpdir.move(to);
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_TRUE(fs::exists(to));
  fs::remove_all(fs::temp_directory_path() / "non-existing");
}
}    // namespace tmp
