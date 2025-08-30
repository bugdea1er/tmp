// SPDX-FileCopyrightText: (c) 2024 Ilya Andreev <bugdealer@icloud.com>
// SPDX-License-Identifier: MIT

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
}

/// Tests multiple file creation
TEST(file, create_multiple) {
  file fst = file();
  file snd = file();

  EXPECT_NE(fst.native_handle(), snd.native_handle());
}

/// Tests file open mode
TEST(file, openmode) {
  file tmpfile = file();
  tmpfile << "Hello, World!" << std::flush;
  tmpfile.seekg(0);
  tmpfile << "Goodbye!" << std::flush;

  tmpfile.seekg(0);
  std::string content = std::string(std::istreambuf_iterator(tmpfile), {});
  EXPECT_EQ(content, "Goodbye!orld!");
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
}    // namespace
}    // namespace tmp
