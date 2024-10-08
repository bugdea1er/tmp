#include <tmp/directory>
#include <tmp/entry>
#include <tmp/file>

#include "utils.hpp"

#include <gtest/gtest.h>
#include <filesystem>

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Returns a temporary file containing `Hello world!`
file test_file() {
  file tmpfile = file();
  tmpfile.write("Hello, world!");

  return tmpfile;
}

/// Returns a temporary directory with a file containing `Hello world!`
directory test_directory() {
  directory tmpdir = directory();
  std::ofstream(tmpdir / "file") << "Hello, world!";

  return tmpdir;
}
}    // namespace

/// Tests that moving a temporary file to itself does nothing
TEST(entry, move_file_to_self) {
  fs::path path;
  entry::native_handle_type handle;

  {
    file tmpfile = test_file();
    path = tmpfile;
    handle = tmpfile.native_handle();

    tmpfile.move(tmpfile);
  }

  EXPECT_TRUE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));

  auto stream = std::ifstream(path);
  auto content = std::string(std::istreambuf_iterator<char>(stream), {});
  EXPECT_EQ(content, "Hello, world!");

  fs::remove_all(path);
}

/// Tests moving a temporary file to existing non-directory file
TEST(entry, move_file_to_existing_file) {
  fs::path path;
  entry::native_handle_type handle;

  fs::path to = fs::path(BUILD_DIR) / "move_file_to_existing_test";
  std::ofstream(to / "file") << "Goodbye, world!";

  {
    file tmpfile = test_file();
    path = tmpfile;
    handle = tmpfile.native_handle();

    tmpfile.move(to);
  }

  EXPECT_TRUE(fs::exists(to));
  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));

  auto stream = std::ifstream(to);
  auto content = std::string(std::istreambuf_iterator<char>(stream), {});
  EXPECT_EQ(content, "Hello, world!");

  fs::remove_all(path);
}

/// Tests moving a temporary file to an existing directory
TEST(entry, move_file_to_existing_directory) {
  fs::path path;
  entry::native_handle_type handle;

  fs::path directory = fs::path(BUILD_DIR) / "existing";
  fs::create_directories(directory);

  EXPECT_THROW(test_file().move(directory), fs::filesystem_error);

  fs::remove_all(directory);
}

/// Tests moving a temporary file to non-existing path
TEST(entry, move_file_to_non_existing_path) {
  fs::path path;
  entry::native_handle_type handle;

  fs::path parent = fs::path(BUILD_DIR) / "non-existing";
  fs::path to = parent / "path";

  {
    file tmpfile = test_file();
    path = tmpfile;
    handle = tmpfile.native_handle();

    tmpfile.move(to);
  }

  EXPECT_TRUE(fs::exists(to));
  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));

  auto stream = std::ifstream(to);
  auto content = std::string(std::istreambuf_iterator<char>(stream), {});
  EXPECT_EQ(content, "Hello, world!");

  fs::remove_all(parent);
}

/// Tests that moving a temporary directory to itself does nothing
TEST(entry, move_directory_to_self) {
  fs::path path;
  entry::native_handle_type handle;

  {
    directory tmpdir = test_directory();
    path = tmpdir;
    handle = tmpdir.native_handle();

    tmpdir.move(tmpdir);
  }

  EXPECT_TRUE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));

  auto stream = std::ifstream(path / "file");
  auto content = std::string(std::istreambuf_iterator<char>(stream), {});
  EXPECT_EQ(content, "Hello, world!");

  fs::remove_all(path);
}
}    // namespace tmp
