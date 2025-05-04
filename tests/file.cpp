#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <iterator>
#include <stdexcept>
#include <system_error>
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
template<class charT, class traits>
bool is_open(const basic_file<charT, traits>& file) {
  auto filebuf = dynamic_cast<std::basic_filebuf<charT, traits>*>(file.rdbuf());
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

template<typename charT>
constexpr std::basic_string<charT> convert_string(const char* string) {
  if constexpr (std::is_same_v<charT, char>) {
    return std::basic_string<charT>(string);
  }

  if constexpr (std::is_same_v<charT, wchar_t>) {
    std::mbstate_t state = std::mbstate_t();

    std::basic_string<charT> result;
    result.resize(std::mbsrtowcs(nullptr, &string, 0, &state));

    std::mbsrtowcs(result.data(), &string, result.size(), &state);
    return result;
  }

  throw std::invalid_argument("Unknown character type");
}

template<typename charT> class file : public testing::Test {};

using char_types = testing::Types<char, wchar_t>;
TYPED_TEST_SUITE(file, char_types, testing::internal::DefaultNameGenerator);

/// Tests file type traits and member types
TYPED_TEST(file, type_traits) {
  using traits = std::char_traits<TypeParam>;

  static_assert(std::is_base_of_v<std::basic_iostream<TypeParam>, basic_file<TypeParam>>);
  static_assert(std::is_same_v<typename basic_file<TypeParam>::char_type, TypeParam>);
  static_assert(std::is_same_v<typename basic_file<TypeParam>::traits_type, traits>);
  static_assert(std::is_same_v<typename basic_file<TypeParam>::int_type, typename traits::int_type>);
  static_assert(std::is_same_v<typename basic_file<TypeParam>::pos_type, typename traits::pos_type>);
  static_assert(std::is_same_v<typename basic_file<TypeParam>::off_type, typename traits::off_type>);
}

/// Tests file creation
TYPED_TEST(file, create) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
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
TYPED_TEST(file, create_multiple) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  basic_file<TypeParam> snd = basic_file<TypeParam>();

  EXPECT_NE(fst.native_handle(), snd.native_handle());
}

/// Tests error handling with invalid open mode
TYPED_TEST(file, create_invalid_openmode) {
  // C++ standard forbids opening a filebuf with `trunc | app`
  EXPECT_THROW(basic_file<TypeParam>(std::ios::trunc | std::ios::app), fs::filesystem_error);
}

/// Tests that file adds std::ios::in and std::ios::out flags
TYPED_TEST(file, ios_flags) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>(std::ios::binary);
  tmpfile << "Hello, world!" << std::flush;

  tmpfile.seekg(0, std::ios::beg);
  std::string content = std::string(std::istreambuf_iterator(tmpfile), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of a temporary copy of a file
TYPED_TEST(file, copy_file) {
  std::ofstream original = std::ofstream("existing.txt", std::ios::binary);
  original << "Hello, world!" << std::flush;

  basic_file<TypeParam> copy = basic_file<TypeParam>::copy("existing.txt");
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
TYPED_TEST(file, copy_errors) {
  // `file::copy` cannot copy a directory
  EXPECT_THROW(basic_file<TypeParam>::copy(directory()), fs::filesystem_error);

  // `file::copy` cannot copy a non-existent file
  EXPECT_THROW(basic_file<TypeParam>::copy("nonexistent.txt"), fs::filesystem_error);
}

/// Tests that destructor removes a file
TYPED_TEST(file, destructor) {
  typename basic_file<TypeParam>::native_handle_type handle;

  {
    basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
    handle = tmpfile.native_handle();
  }

  EXPECT_FALSE(is_open(handle));
}

/// Tests file move constructor
TYPED_TEST(file, move_constructor) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  fst << "Hello!";

  basic_file<TypeParam> snd = basic_file<TypeParam>(std::move(fst));

  EXPECT_TRUE(is_open(snd));

  snd.seekg(0);
  std::basic_string<TypeParam> content;
  snd >> content;
  EXPECT_EQ(content, convert_string<TypeParam>("Hello!"));
}

/// Tests file move assignment operator
TYPED_TEST(file, move_assignment) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();

  {
    basic_file<TypeParam> snd = basic_file<TypeParam>();
    snd << "Hello!";

    typename basic_file<TypeParam>::native_handle_type fst_handle = fst.native_handle();
    typename basic_file<TypeParam>::native_handle_type snd_handle = snd.native_handle();

    fst = std::move(snd);

    EXPECT_FALSE(is_open(fst_handle));
    EXPECT_TRUE(is_open(snd_handle));
    EXPECT_EQ(fst.native_handle(), snd_handle);
  }

  EXPECT_TRUE(is_open(fst));

  fst.seekg(0);
  std::basic_string<TypeParam> content;
  fst >> content;
  EXPECT_EQ(content, convert_string<TypeParam>("Hello!"));
}

/// Tests file swapping
TYPED_TEST(file, swap) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  basic_file<TypeParam> snd = basic_file<TypeParam>();

  typename basic_file<TypeParam>::native_handle_type fst_handle = fst.native_handle();
  typename basic_file<TypeParam>::native_handle_type snd_handle = snd.native_handle();

  std::swap(fst, snd);

  EXPECT_EQ(fst.native_handle(), snd_handle);
  EXPECT_EQ(snd.native_handle(), fst_handle);
  EXPECT_TRUE(is_open(fst));
  EXPECT_TRUE(is_open(snd));
}

/// Tests file hashing
TYPED_TEST(file, hash) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
  std::hash hash = std::hash<basic_file<TypeParam>>();

  EXPECT_EQ(hash(tmpfile), std::hash<typename basic_file<TypeParam>::native_handle_type>()(tmpfile.native_handle()));
}
}    // namespace
}    // namespace tmp
