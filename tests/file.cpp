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

/// Test fixture for `basic_file` tests
template<class charT> class file : public testing::Test {
public:
  /// Returns whether the file device object is open
  bool is_open(const std::basic_streambuf<charT>* streambuf) {
    auto filebuf = dynamic_cast<const std::basic_filebuf<charT>*>(streambuf);
    return filebuf != nullptr && filebuf->is_open();
  }

  /// Returns whether the given file handle is open
  bool is_open(typename basic_file<charT>::native_handle_type handle) {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION info;
    return GetFileInformationByHandle(handle, &info);
#else
    return fcntl(handle, F_GETFD) != -1;
#endif
  }

  /// Converts the string to the `charT` string type
  constexpr std::basic_string<charT> convert_string(std::string_view string) {
    std::basic_string<charT> result;

    for (char character : string) {
      result += static_cast<charT>(character);
    }

    return result;
  }
};

/// Name generator for typed test suite
struct name_generator {
  template<typename T> static std::string GetName(int) {
    return typeid(T).name();
  }
};

using char_types = testing::Types<char, wchar_t>;
TYPED_TEST_SUITE(file, char_types, name_generator);

/// Tests file type traits and member types
TYPED_TEST(file, type_traits) {
  using traits = std::char_traits<TypeParam>;
  using file_t = basic_file<TypeParam>;
  using testing::StaticAssertTypeEq;

  static_assert(std::is_base_of_v<std::basic_iostream<TypeParam>, file_t>);
  StaticAssertTypeEq<typename file_t::char_type, TypeParam>();
  StaticAssertTypeEq<typename file_t::traits_type, traits>();
  StaticAssertTypeEq<typename file_t::int_type, typename traits::int_type>();
  StaticAssertTypeEq<typename file_t::pos_type, typename traits::pos_type>();
  StaticAssertTypeEq<typename file_t::off_type, typename traits::off_type>();
}

/// Tests file creation
TYPED_TEST(file, create) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
  EXPECT_TRUE(TestFixture::is_open(tmpfile.rdbuf()));
  EXPECT_TRUE(TestFixture::is_open(tmpfile.native_handle()));
}

/// Tests multiple file creation
TYPED_TEST(file, create_multiple) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  basic_file<TypeParam> snd = basic_file<TypeParam>();

  EXPECT_NE(fst.native_handle(), snd.native_handle());
}

/// Tests file open mode
TYPED_TEST(file, openmode) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
  tmpfile << "Hello, World!" << std::flush;
  tmpfile.seekg(0);
  tmpfile << "Goodbye!" << std::flush;

  tmpfile.seekg(0);
  std::string content = std::string(std::istreambuf_iterator(tmpfile), {});
  EXPECT_EQ(content, "Goodbye!orld!");
}

/// Tests that destructor removes a file
TYPED_TEST(file, destructor) {
  typename basic_file<TypeParam>::native_handle_type handle;

  {
    basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
    handle = tmpfile.native_handle();
  }

  EXPECT_FALSE(TestFixture::is_open(handle));
}

/// Tests file move constructor
TYPED_TEST(file, move_constructor) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  fst << "Hello!";

  basic_file<TypeParam> snd = basic_file<TypeParam>(std::move(fst));

  EXPECT_TRUE(TestFixture::is_open(snd.rdbuf()));

  snd.seekg(0);
  std::basic_string<TypeParam> content;
  snd >> content;
  EXPECT_EQ(content, TestFixture::convert_string("Hello!"));
}

/// Tests file move assignment operator
TYPED_TEST(file, move_assignment) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();

  {
    basic_file<TypeParam> snd = basic_file<TypeParam>();
    snd << "Hello!";

    typename decltype(fst)::native_handle_type fst_handle = fst.native_handle();
    typename decltype(snd)::native_handle_type snd_handle = snd.native_handle();

    fst = std::move(snd);

    EXPECT_FALSE(TestFixture::is_open(fst_handle));
    EXPECT_TRUE(TestFixture::is_open(snd_handle));
    EXPECT_EQ(fst.native_handle(), snd_handle);
  }

  EXPECT_TRUE(TestFixture::is_open(fst.rdbuf()));

  fst.seekg(0);
  std::basic_string<TypeParam> content;
  fst >> content;
  EXPECT_EQ(content, TestFixture::convert_string("Hello!"));
}

/// Tests file swapping
TYPED_TEST(file, swap) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  basic_file<TypeParam> snd = basic_file<TypeParam>();

  typename decltype(fst)::native_handle_type fst_handle = fst.native_handle();
  typename decltype(snd)::native_handle_type snd_handle = snd.native_handle();

  std::swap(fst, snd);

  EXPECT_EQ(fst.native_handle(), snd_handle);
  EXPECT_EQ(snd.native_handle(), fst_handle);
  EXPECT_TRUE(TestFixture::is_open(fst.rdbuf()));
  EXPECT_TRUE(TestFixture::is_open(snd.rdbuf()));
}
}    // namespace
}    // namespace tmp
