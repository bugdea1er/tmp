#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <iterator>
#include <ostream>
#include <system_error>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#define UNICODE
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
  EXPECT_TRUE(is_open(tmpfile));
  EXPECT_TRUE(is_open(tmpfile.native_handle()));

#ifdef _WIN32
  BY_HANDLE_FILE_INFORMATION file_info;
  GetFileInformationByHandle(tmpfile.native_handle(), &file_info);

  BY_HANDLE_FILE_INFORMATION temp_directory_info;
  HANDLE temp_directory_handle =
      CreateFile(fs::temp_directory_path().c_str(), FILE_READ_ATTRIBUTES,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                 FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, NULL);
  GetFileInformationByHandle(temp_directory_handle, &temp_directory_info);
  CloseHandle(temp_directory_handle);

  EXPECT_EQ(file_info.dwVolumeSerialNumber,
            temp_directory_info.dwVolumeSerialNumber);
  EXPECT_EQ(file_info.nNumberOfLinks, 0);    // Has no hardlinks
#else
  struct stat file_stat;
  fstat(tmpfile.native_handle(), &file_stat);

  struct stat temp_directory_stat;
  stat(fs::temp_directory_path().c_str(), &temp_directory_stat);

  EXPECT_EQ(file_stat.st_dev, temp_directory_stat.st_dev);
  EXPECT_EQ(file_stat.st_mode & S_IFMT, S_IFREG);    // Is a regular file
  EXPECT_EQ(file_stat.st_nlink, 0);                  // Has no hardlinks
#endif
}

/// Tests multiple file creation
TEST(file, create_multiple) {
  file fst = file();
  file snd = file();

  EXPECT_NE(fst.native_handle(), snd.native_handle());
}

/// Tests error handling with invalid open mode
TEST(file, create_invalid_openmode) {
  // C++ standard forbids opening a filebuf with `trunc | app`
  EXPECT_THROW(file(std::ios::trunc | std::ios::app), std::invalid_argument);
}

/// Tests file default open mode
TEST(file, openmode_default) {
  file tmpfile = file();
  tmpfile << "Hello, World!" << std::flush;
  tmpfile.seekg(0);
  tmpfile << "Goodbye!" << std::flush;

  tmpfile.seekg(0);
  std::string content = std::string(std::istreambuf_iterator(tmpfile), {});
  EXPECT_EQ(content, "Goodbye!orld!");
}

/// Tests file `app` open mode
TEST(file, openmode_append) {
  file tmpfile = file(std::ios::app);
  tmpfile << "Hello, World!" << std::flush;
  tmpfile.seekg(0);
  tmpfile << "Goodbye!" << std::flush;

  tmpfile.seekg(0);
  std::string content = std::string(std::istreambuf_iterator(tmpfile), {});
  EXPECT_EQ(content, "Hello, World!Goodbye!");
}

/// Tests that file adds std::ios::in and std::ios::out flags
TEST(file, ios_flags) {
  file tmpfile = file(std::ios::binary);
  tmpfile << "Hello, world!" << std::flush;

  tmpfile.seekg(0, std::ios::beg);
  std::string content = std::string(std::istreambuf_iterator(tmpfile), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of a temporary copy of a file
TEST(file, copy_empty_file) {
  std::ofstream original = std::ofstream("empty.txt", std::ios::binary);

  file copy = file::copy("empty.txt");
  EXPECT_TRUE(fs::exists("empty.txt"));
  EXPECT_TRUE(is_open(copy));

  // Test file copy contents
  copy.seekg(0, std::ios::beg);
  std::string content = std::string(std::istreambuf_iterator(copy), {});
  EXPECT_EQ(content, "");
}

/// Tests creation of a temporary copy of a file
TEST(file, copy_non_empty_file) {
  std::ofstream original = std::ofstream("existing.txt", std::ios::binary);
  original << "Hello, world!" << std::flush;

  file copy = file::copy("existing.txt");
  EXPECT_TRUE(fs::exists("existing.txt"));
  EXPECT_TRUE(is_open(copy));

  // To test that we actually copy the original file
  original << "Goodbye, world!" << std::flush;

  // Test get file pointer position after copying
  std::streampos gstreampos = copy.tellg();
  copy.seekg(0, std::ios::end);
  EXPECT_EQ(gstreampos, copy.tellg());

  // Test put file pointer position after copying
  std::streampos pstreampos = copy.tellp();
  copy.seekp(0, std::ios::end);
  EXPECT_EQ(pstreampos, copy.tellp());

  // Test file copy contents
  copy.seekg(0, std::ios::beg);
  std::string content = std::string(std::istreambuf_iterator(copy), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of copy errors
TEST(file, copy_errors) {
  try {
    file::copy("nonexistent.txt");
    FAIL() << "Expected exception";
  } catch (const fs::filesystem_error& ex) {
    EXPECT_EQ(ex.path1(), "nonexistent.txt");
    EXPECT_EQ(ex.path2(), fs::path());
  }
}

/// Tests that destructor removes a file
TEST(file, destructor) {
  file::native_handle_type handle;

  {
    file tmpfile = file();
    handle = tmpfile.native_handle();
  }

  EXPECT_FALSE(is_open(handle));
}

/// Tests file move constructor
TEST(file, move_constructor) {
  file fst = file();
  fst << "Hello!";

  file snd = file(std::move(fst));

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

    file::native_handle_type fst_handle = fst.native_handle();
    file::native_handle_type snd_handle = snd.native_handle();

    fst = std::move(snd);

    EXPECT_FALSE(is_open(fst_handle));
    EXPECT_TRUE(is_open(snd_handle));
    EXPECT_EQ(fst.native_handle(), snd_handle);
  }

  EXPECT_TRUE(is_open(fst));

  fst.seekg(0);
  std::string content;
  fst >> content;
  EXPECT_EQ(content, "Hello!");
}

/// Tests file swapping
TEST(file, swap) {
  file fst = file();
  file snd = file();

  file::native_handle_type fst_handle = fst.native_handle();
  file::native_handle_type snd_handle = snd.native_handle();

  std::swap(fst, snd);

  EXPECT_EQ(fst.native_handle(), snd_handle);
  EXPECT_EQ(snd.native_handle(), fst_handle);
  EXPECT_TRUE(is_open(fst));
  EXPECT_TRUE(is_open(snd));
}

/// Tests file hashing
TEST(file, hash) {
  file tmpfile = file();
  std::hash hash = std::hash<file>();

  EXPECT_EQ(hash(tmpfile),
            std::hash<file::native_handle_type>()(tmpfile.native_handle()));
}
}    // namespace
}    // namespace tmp
