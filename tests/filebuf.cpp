#include <tmp/filebuf>

#include <gtest/gtest.h>

#include <cwchar>
#include <filesystem>
#include <string>

namespace tmp {
namespace {

template<typename charT>
constexpr std::basic_string<charT> convert_string(const char* string) {
  if constexpr (std::is_same_v<charT, char>) {
    return std::basic_string<charT>(string);
  }

  if constexpr (std::is_same_v<charT, wchar_t>) {
    std::mbstate_t state = std::mbstate_t();

    std::basic_string<charT> result;
    result.resize(std::mbsrtowcs(nullptr, &string, 0, &state) + 1);

    std::mbsrtowcs(result.data(), &string, result.size(), &state);
    return result;
  }

  throw std::invalid_argument("Unknown character type");
}

template<typename charT> class filebuf : public testing::Test {};

using char_types = testing::Types<char, wchar_t>;
TYPED_TEST_SUITE(filebuf, char_types);

/// Tests basic_filebuf member types
TYPED_TEST(filebuf, type_traits) {
  using traits = std::char_traits<TypeParam>;
  using filebuf = basic_filebuf<TypeParam>;

  static_assert(std::is_base_of_v<std::basic_streambuf<TypeParam>, filebuf>);
  static_assert(std::is_same_v<typename filebuf::char_type, TypeParam>);
  static_assert(std::is_same_v<typename filebuf::traits_type, traits>);
  static_assert(std::is_same_v<typename filebuf::int_type, typename traits::int_type>);
  static_assert(std::is_same_v<typename filebuf::pos_type, typename traits::pos_type>);
  static_assert(std::is_same_v<typename filebuf::off_type, typename traits::off_type>);
}

/// Tests basic_filebuf constructor
TYPED_TEST(filebuf, constructor) {
  auto sb = basic_filebuf<TypeParam>(std::tmpfile(), std::ios::in | std::ios::out);
  std::basic_string<TypeParam> string = convert_string<TypeParam>("123");

  EXPECT_TRUE(sb.is_open());
  EXPECT_EQ(sb.sputn(string.data(), 3), 3);

  sb.pubseekoff(0, std::ios::beg, std::ios::in);

  EXPECT_EQ(sb.sbumpc(), '1');
  EXPECT_EQ(sb.sbumpc(), '2');
  EXPECT_EQ(sb.sbumpc(), '3');
}

/// Tests basic_filebuf move constructor
TYPED_TEST(filebuf, move_constructor) {
  auto fst = basic_filebuf<TypeParam>(std::tmpfile(), std::ios::in | std::ios::out);
  std::basic_string<TypeParam> string = convert_string<TypeParam>("123");

  EXPECT_TRUE(fst.is_open());
  EXPECT_EQ(fst.sputn(string.data(), 3), 3);
  fst.pubseekoff(1, std::ios::beg);
  EXPECT_EQ(fst.sgetc(), '2');

  basic_filebuf<TypeParam> snd = std::move(fst);
  EXPECT_FALSE(fst.is_open());
  EXPECT_TRUE(snd.is_open());
  EXPECT_EQ(snd.sgetc(), '2');
}

/// Tests filebuf move assignment operator
TYPED_TEST(filebuf, move_assignment) {
  auto fst = basic_filebuf<TypeParam>(std::tmpfile(), std::ios::in | std::ios::out);
  std::basic_string<TypeParam> string = convert_string<TypeParam>("123");

  EXPECT_TRUE(fst.is_open());
  EXPECT_EQ(fst.sputn(string.data(), 3), 3);
  fst.pubseekoff(1, std::ios_base::beg);
  EXPECT_EQ(fst.sgetc(), '2');

  auto snd = basic_filebuf<TypeParam>(std::tmpfile(), std::ios::in | std::ios::out);
  snd = std::move(fst);
  EXPECT_FALSE(fst.is_open());
  EXPECT_TRUE(snd.is_open());
  EXPECT_EQ(snd.sgetc(), '2');
}

/// Tests filebuf swapping
TYPED_TEST(filebuf, swap) {
  auto fst = basic_filebuf<TypeParam>(std::tmpfile(), std::ios::in | std::ios::out);
  std::basic_string<TypeParam> string = convert_string<TypeParam>("123");

  EXPECT_TRUE(fst.is_open());
  EXPECT_EQ(fst.sputn(string.data(), 3), 3);
  fst.pubseekoff(1, std::ios::beg);
  EXPECT_EQ(fst.sgetc(), '2');

  auto snd = basic_filebuf<TypeParam>(std::tmpfile(), std::ios::in | std::ios::out);
  snd.close();

  std::swap(fst, snd);
  EXPECT_FALSE(fst.is_open());
  EXPECT_TRUE(snd.is_open());
  EXPECT_EQ(snd.sgetc(), '2');
}
}    // namespace
}    // namespace tmp
