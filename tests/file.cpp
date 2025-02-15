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
  std::filebuf* filebuf = dynamic_cast<std::filebuf*>(file.rdbuf());
  return filebuf != nullptr && filebuf->is_open();
}

/// Checks if the given file handle is valid
/// @param handle handle to check
/// @returns whether the handle is valid
bool is_open(file::native_handle_type handle) {
#ifdef _WIN32
  BY_HANDLE_FILE_INFORMATION info;
  return GetFileInformationByHandle(handle, &info);
#else
  return fcntl(handle, F_GETFD) != -1;
#endif
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

/// Tests file creation
TEST(file, create) {
  file tmpfile = file();
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
  EXPECT_TRUE(is_open(tmpfile));
  EXPECT_TRUE(is_open(tmpfile.native_handle()));

  fs::perms permissions = fs::status(tmpfile).permissions();
#ifdef _WIN32
  // GetTempFileNameW creates a file with all permissions
  EXPECT_EQ(permissions, fs::perms::all);
#else
  // mkstemp creates a file that can only be read and written by the owner
  EXPECT_EQ(permissions, fs::perms::owner_read | fs::perms::owner_write);
#endif
}

/// Tests multiple file creation
TEST(file, create_multiple) {
  file fst = file();
  file snd = file();

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests error handling with invalid open mode
TEST(file, create_invalid_openmode) {
  // C++ standard forbids opening a filebuf with `trunc | app`
  EXPECT_THROW(file(std::ios::trunc | std::ios::app), fs::filesystem_error);
}

/// Tests that file adds std::ios::in and std::ios::out flags
TEST(file, ios_flags) {
  file tmpfile = file(std::ios::binary);
  tmpfile << "Hello, world!" << std::flush;

  std::ifstream stream = std::ifstream(tmpfile.path());
  std::string content = std::string(std::istreambuf_iterator(stream), {});
  EXPECT_EQ(content, "Hello, world!");
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

  // Test get file pointer position after copying
  std::streampos gstreampos = copy.tellg();
  copy.seekg(0, std::ios::end);
  EXPECT_EQ(gstreampos, copy.tellg());

  // Test put file pointer position after copying
  std::streampos pstreampos = copy.tellp();
  copy.seekp(0, std::ios::end);
  EXPECT_EQ(pstreampos, copy.tellp());

  // Test file copy contents
  std::ifstream stream = std::ifstream(copy.path());
  std::string content = std::string(std::istreambuf_iterator(stream), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of copy errors
TEST(file, copy_errors) {
  // `file::copy` cannot copy a directory
  EXPECT_THROW(file::copy(directory()), fs::filesystem_error);

  // `file::copy` cannot copy a non-existent file
  EXPECT_THROW(file::copy("nonexistent.txt"), fs::filesystem_error);
}

/// Tests moving a temporary file to existing non-directory file
TEST(file, move_to_existing_file) {
  fs::path to = fs::path(BUILD_DIR) / "move_file_to_existing_test";
  std::ofstream(to) << "Goodbye, world!";

  {
    file tmpfile = file();
    tmpfile << "Hello, world!" << std::flush;

    tmpfile.move(to);
  }

  std::error_code ec;
  EXPECT_TRUE(fs::exists(to, ec));

  {
    std::ifstream stream = std::ifstream(to);
    std::string content = std::string(std::istreambuf_iterator(stream), {});
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
  file::native_handle_type handle;

  {
    file tmpfile = file();
    path = tmpfile;
    handle = tmpfile.native_handle();
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(is_open(handle));
}

/// Tests file move constructor
TEST(file, move_constructor) {
  file fst = file();
  fst << "Hello!";

  file snd = file(std::move(fst));

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
  EXPECT_TRUE(is_open(snd));

  snd.seekg(0);
  std::string content;
  snd >> content;
  EXPECT_EQ(content, "Hello!");
}

/// Tests file move assignment operator
TEST(file, move_assignment) {
  file fst = file();

  {
    file snd = file();
    snd << "Hello!";

    fs::path path1 = fst;
    fs::path path2 = snd;

    fst = std::move(snd);

    EXPECT_FALSE(fs::exists(path1));
    EXPECT_TRUE(fs::exists(path2));

    EXPECT_TRUE(fs::exists(fst));
    EXPECT_TRUE(fs::equivalent(fst, path2));
  }

  EXPECT_FALSE(fst.path().empty());

  fst.seekg(0);
  std::string content;
  fst >> content;
  EXPECT_EQ(content, "Hello!");
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
