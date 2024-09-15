#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#endif

namespace tmp {

namespace fs = std::filesystem;

namespace {
/// Checks if the given file handle is valid
/// @param handle handle to check
/// @returns @c true if the handle is valid, @c false otherwise
bool native_handle_is_valid(entry::native_handle_type handle) {
#ifdef _WIN32
  BY_HANDLE_FILE_INFORMATION info;
  return GetFileInformationByHandle(handle, &info);
#else
  return fcntl(handle, F_GETFD) != -1;
#endif
}
}    // namespace

/// Tests directory creation with label
TEST(directory, create_with_label) {
  directory tmpdir = directory(LABEL);
  fs::path parent = tmpdir.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::is_directory(tmpdir));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / LABEL));
  EXPECT_TRUE(native_handle_is_valid(tmpdir.native_handle()));

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
  EXPECT_TRUE(native_handle_is_valid(tmpdir.native_handle()));
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

  std::set entries = std::set<fs::path>();
  for (const auto& entry : tmpdir.list()) {
    entries.insert(fs::relative(entry, tmpdir));
  }

  EXPECT_EQ(entries, std::set<fs::path>({"file1", "file2", "subdir"}));
}

/// Tests that destructor removes a directory
TEST(directory, destructor) {
  fs::path path = fs::path();
  entry::native_handle_type handle;
  {
    directory tmpdir = directory();
    path = tmpdir;
    handle = tmpdir.native_handle();
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));
}

/// Tests directory move constructor
TEST(directory, move_constructor) {
  directory fst = directory();
  directory snd = std::move(fst);

  EXPECT_TRUE(fst.path().empty());
  EXPECT_TRUE(fs::exists(snd));
  EXPECT_TRUE(native_handle_is_valid(snd.native_handle()));
}

/// Tests directory move assignment operator
TEST(directory, move_assignment) {
  directory fst = directory();
  directory snd = directory();

  fs::path path1 = fst;
  fs::path path2 = snd;

  entry::native_handle_type fst_handle = fst.native_handle();
  entry::native_handle_type snd_handle = snd.native_handle();

  fst = std::move(snd);

  EXPECT_FALSE(fs::exists(path1));
  EXPECT_TRUE(fs::exists(path2));

  EXPECT_TRUE(fs::exists(fst));
  EXPECT_TRUE(fs::equivalent(fst, path2));

  EXPECT_FALSE(native_handle_is_valid(fst_handle));
  EXPECT_TRUE(native_handle_is_valid(snd_handle));
}

/// Tests directory moving
TEST(directory, move) {
  fs::path path = fs::path();
  entry::native_handle_type handle;

  fs::path to = fs::temp_directory_path() / "non-existing" / "parent";
  {
    directory tmpdir = directory();
    path = tmpdir;
    handle = tmpdir.native_handle();

    tmpdir.move(to);
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_TRUE(fs::exists(to));
  EXPECT_FALSE(native_handle_is_valid(handle));

  fs::remove_all(fs::temp_directory_path() / "non-existing");
}

/// Tests directory swapping
TEST(directory, swap) {
  directory fst = directory();
  directory snd = directory();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();
  entry::native_handle_type fst_handle = fst.native_handle();
  entry::native_handle_type snd_handle = snd.native_handle();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
  EXPECT_EQ(fst.native_handle(), snd_handle);
  EXPECT_EQ(snd.native_handle(), fst_handle);
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
}    // namespace tmp
