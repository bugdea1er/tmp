#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Returns whether the underlying raw file device object is open
bool is_open(const file& file) {
  return file.rdbuf()->is_open();
}

/// Tests file type traits and member types
TEST(file, type_traits) {
  using traits = std::char_traits<char>;

  static_assert(std::is_base_of_v<std::basic_iostream<char>, file>);
  static_assert(std::is_same_v<file::char_type, char>);
  static_assert(std::is_same_v<file::traits_type, traits>);
  static_assert(std::is_same_v<file::int_type, traits::int_type>);
  static_assert(std::is_same_v<file::pos_type, traits::pos_type>);
  static_assert(std::is_same_v<file::off_type, traits::off_type>);
}

/// Tests file creation with label
TEST(file, create_with_label) {
  file tmpfile = file(LABEL);
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / LABEL));
  EXPECT_TRUE(is_open(tmpfile));

  fs::perms permissions = fs::status(tmpfile).permissions();
#ifdef _WIN32
  // GetTempFileNameW creates a file with all permissions
  EXPECT_EQ(permissions, fs::perms::all);
#else
  // mkstemp creates a file that can only be read and written by the owner
  EXPECT_EQ(permissions, fs::perms::owner_read | fs::perms::owner_write);
#endif
}

/// Tests file creation without label
TEST(file, create_without_label) {
  file tmpfile = file();
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
  EXPECT_TRUE(is_open(tmpfile));
}

/// Tests file creation with extension
TEST(file, create_with_extension) {
  file tmpfile = file("", ".test");

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_EQ(tmpfile.path().extension(), ".test");
  EXPECT_TRUE(is_open(tmpfile));
}

/// Tests multiple file creation with the same label
TEST(file, create_multiple) {
  file fst = file(LABEL);
  file snd = file(LABEL);

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests error handling with invalid labels
TEST(file, create_invalid_label) {
  EXPECT_THROW(file("multi/segment"), std::invalid_argument);
  EXPECT_THROW(file("/root"), std::invalid_argument);
  EXPECT_THROW(file(".."), std::invalid_argument);
  EXPECT_THROW(file("."), std::invalid_argument);

  fs::path root = fs::temp_directory_path().root_name();
  if (!root.empty()) {
    EXPECT_THROW(file(root.string() + "relative"), std::invalid_argument);
    EXPECT_THROW(file(root.string() + "/root"), std::invalid_argument);
  }
}

/// Tests error handling with invalid extensions
TEST(file, create_invalid_extension) {
  EXPECT_THROW(file("", "multi/segment"), std::invalid_argument);
  EXPECT_THROW(file("", "/root"), std::invalid_argument);
  EXPECT_THROW(file("", "/.."), std::invalid_argument);
  EXPECT_THROW(file("", "/."), std::invalid_argument);
}

/// Tests creation of a temporary copy of a file
TEST(file, copy_file) {
  file tmpfile = file();
  tmpfile << "Hello, world!" << std::flush;

  file copy = file::copy(tmpfile);
  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::exists(copy));
  EXPECT_FALSE(fs::equivalent(tmpfile, copy));

  EXPECT_TRUE(fs::is_regular_file(tmpfile));

  auto stream = std::ifstream(copy.path());
  auto content = std::string(std::istreambuf_iterator<char>(stream), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of a temporary copy of a directory
TEST(file, copy_directory) {
  directory tmpdir = directory();
  EXPECT_THROW(file::copy(tmpdir), fs::filesystem_error);
}

/// Tests that moving a temporary file to itself does nothing
TEST(file, move_to_self) {
  fs::path path;

  {
    file tmpfile = file();
    tmpfile << "Hello, world!" << std::flush;

    path = tmpfile;

    tmpfile.move(tmpfile);
  }

  EXPECT_TRUE(fs::exists(path));

  {
    auto stream = std::ifstream(path);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }

  fs::remove_all(path);
}

/// Tests moving a temporary file to existing non-directory file
TEST(file, move_to_existing_file) {
  fs::path path;

  fs::path to = fs::path(BUILD_DIR) / "move_file_to_existing_test";
  std::ofstream(to) << "Goodbye, world!";

  {
    file tmpfile = file();
    tmpfile << "Hello, world!" << std::flush;

    path = tmpfile;

    tmpfile.move(to);
  }

  std::error_code ec;
  EXPECT_TRUE(fs::exists(to, ec));
  EXPECT_FALSE(fs::exists(path, ec));

  {
    auto stream = std::ifstream(to);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }

  fs::remove_all(to);
}

/// Tests moving a temporary file to an existing directory
TEST(file, move_to_existing_directory) {
  fs::path directory = fs::path(BUILD_DIR) / "existing_directory";
  fs::create_directories(directory);

  EXPECT_THROW(file().move(directory), fs::filesystem_error);

  fs::remove_all(directory);
}

/// Tests moving a temporary file to a non-existing directory
TEST(file, move_to_non_existing_directory) {
  fs::path parent = fs::path(BUILD_DIR) / "non-existing2";
  fs::path to = parent / "path/";

  EXPECT_THROW(file().move(to), fs::filesystem_error);

  fs::remove_all(parent);
}

/// Tests that destructor removes a file
TEST(file, destructor) {
  fs::path path;
  {
    file tmpfile = file();
    path = tmpfile;
  }

  EXPECT_FALSE(fs::exists(path));
}

/// Tests file move constructor
TEST(file, move_constructor) {
  file fst = file();
  file snd = file(std::move(fst));

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
  EXPECT_TRUE(is_open(snd));
}

/// Tests file move assignment operator
TEST(file, move_assignment) {
  file fst = file();
  {
    file snd = file();

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

/// Tests file swapping
TEST(file, swap) {
  file fst = file();
  file snd = file();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
  EXPECT_TRUE(is_open(fst));
  EXPECT_TRUE(is_open(snd));
}

/// Tests file hashing
TEST(file, hash) {
  file tmpfile = file();
  std::hash hash = std::hash<file>();

  EXPECT_EQ(hash(tmpfile), fs::hash_value(tmpfile.path()));
}

/// Tests file relational operators
TEST(file, relational) {
  file tmpfile = file();

  EXPECT_TRUE(tmpfile == tmpfile);
  EXPECT_FALSE(tmpfile < tmpfile);
}
}    // namespace
}    // namespace tmp
