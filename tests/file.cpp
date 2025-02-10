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
template<typename charT> bool is_open(const basic_file<charT>& file) {
  return file.rdbuf()->is_open();
}

#if __cpp_lib_fstream_native_handle >= 202306L
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
#endif

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
TYPED_TEST_SUITE(file, char_types);

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

/// Tests file creation with label
TYPED_TEST(file, create_with_label) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>(LABEL);
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / LABEL));
  EXPECT_TRUE(is_open(tmpfile));

#if __cpp_lib_fstream_native_handle >= 202306L
  EXPECT_TRUE(is_open(tmpfile.native_handle()));
#endif

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
TYPED_TEST(file, create_without_label) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
  EXPECT_TRUE(is_open(tmpfile));
}

/// Tests file creation with extension
TYPED_TEST(file, create_with_extension) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>("", ".test");

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_EQ(tmpfile.path().extension(), ".test");
  EXPECT_TRUE(is_open(tmpfile));
}

/// Tests multiple file creation with the same label
TYPED_TEST(file, create_multiple) {
  basic_file<TypeParam> fst = basic_file<TypeParam>(LABEL);
  basic_file<TypeParam> snd = basic_file<TypeParam>(LABEL);

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests error handling with invalid labels
TYPED_TEST(file, create_invalid_label) {
  EXPECT_THROW(basic_file<TypeParam>("multi/segment"), std::invalid_argument);
  EXPECT_THROW(basic_file<TypeParam>("/root"), std::invalid_argument);
  EXPECT_THROW(basic_file<TypeParam>(".."), std::invalid_argument);
  EXPECT_THROW(basic_file<TypeParam>("."), std::invalid_argument);

  fs::path root = fs::temp_directory_path().root_name();
  if (!root.empty()) {
    EXPECT_THROW(basic_file<TypeParam>(root.string() + "relative"), std::invalid_argument);
    EXPECT_THROW(basic_file<TypeParam>(root.string() + "/root"), std::invalid_argument);
  }
}

/// Tests error handling with invalid extensions
TYPED_TEST(file, create_invalid_extension) {
  EXPECT_THROW(basic_file<TypeParam>("", "multi/segment"), std::invalid_argument);
  EXPECT_THROW(basic_file<TypeParam>("", "/root"), std::invalid_argument);
  EXPECT_THROW(basic_file<TypeParam>("", "/.."), std::invalid_argument);
  EXPECT_THROW(basic_file<TypeParam>("", "/."), std::invalid_argument);
}

/// Tests creation of a temporary copy of a file
TYPED_TEST(file, copy_file) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
  tmpfile << "Hello, world!" << std::flush;

  basic_file<TypeParam> copy = basic_file<TypeParam>::copy(tmpfile);
  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::exists(copy));
  EXPECT_FALSE(fs::equivalent(tmpfile, copy));

  EXPECT_TRUE(fs::is_regular_file(tmpfile));

  std::ifstream stream = std::ifstream(copy.path());
  std::string content = std::string(std::istreambuf_iterator(stream), {});
  EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of a temporary copy of a directory
TYPED_TEST(file, copy_directory) {
  directory tmpdir = directory();
  EXPECT_THROW(basic_file<TypeParam>::copy(tmpdir), fs::filesystem_error);
}

/// Tests that moving a temporary file to itself does nothing
TYPED_TEST(file, move_to_self) {
  fs::path path;

  {
    basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
    tmpfile << "Hello, world!";

    path = tmpfile;

    tmpfile.move(tmpfile);
  }

  EXPECT_TRUE(fs::exists(path));

  {
    std::ifstream stream = std::ifstream(path);
    std::string content = std::string(std::istreambuf_iterator(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }

  fs::remove_all(path);
}

/// Tests moving a temporary file to existing non-directory file
TYPED_TEST(file, move_to_existing_file) {
  fs::path path;

  fs::path to = fs::path(BUILD_DIR) / "move_file_to_existing_test";
  std::ofstream(to) << "Goodbye, world!";

  {
    basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
    tmpfile << "Hello, world!";

    path = tmpfile;

    tmpfile.move(to);
  }

  std::error_code ec;
  EXPECT_TRUE(fs::exists(to, ec));
  EXPECT_FALSE(fs::exists(path, ec));

  {
    std::ifstream stream = std::ifstream(to);
    std::string content = std::string(std::istreambuf_iterator(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }

  fs::remove_all(to);
}

/// Tests moving a temporary file to an existing directory
TYPED_TEST(file, move_to_existing_directory) {
  fs::path directory = fs::path(BUILD_DIR) / "existing_directory";
  fs::create_directories(directory);

  EXPECT_THROW(basic_file<TypeParam>().move(directory), fs::filesystem_error);

  fs::remove_all(directory);
}

/// Tests moving a temporary file to a non-existing directory
TYPED_TEST(file, move_to_non_existing_directory) {
  fs::path parent = fs::path(BUILD_DIR) / "non-existing2";
  fs::path to = parent / "path/";

  EXPECT_THROW(basic_file<TypeParam>().move(to), fs::filesystem_error);

  fs::remove_all(parent);
}

/// Tests that destructor removes a file
TYPED_TEST(file, destructor) {
  fs::path path;
#if __cpp_lib_fstream_native_handle >= 202306L
  file::native_handle_type handle;
#endif

  {
    basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
    path = tmpfile;
#if __cpp_lib_fstream_native_handle >= 202306L
    handle = tmpfile.native_handle();
#endif
  }

  EXPECT_FALSE(fs::exists(path));
#if __cpp_lib_fstream_native_handle >= 202306L
  EXPECT_FALSE(is_open(handle));
#endif
}

/// Tests file move constructor
TYPED_TEST(file, move_constructor) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  fst << "Hello!";

  basic_file<TypeParam> snd = basic_file<TypeParam>(std::move(fst));

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
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
  std::basic_string<TypeParam> content;
  fst >> content;
  EXPECT_EQ(content, convert_string<TypeParam>("Hello!"));
}

/// Tests file swapping
TYPED_TEST(file, swap) {
  basic_file<TypeParam> fst = basic_file<TypeParam>();
  basic_file<TypeParam> snd = basic_file<TypeParam>();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
  EXPECT_TRUE(is_open(fst));
  EXPECT_TRUE(is_open(snd));
}

/// Tests file hashing
TYPED_TEST(file, hash) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();
  std::hash hash = std::hash<basic_file<TypeParam>>();

  EXPECT_EQ(hash(tmpfile), fs::hash_value(tmpfile.path()));
}

/// Tests file relational operators
TYPED_TEST(file, relational) {
  basic_file<TypeParam> tmpfile = basic_file<TypeParam>();

  EXPECT_TRUE(tmpfile == tmpfile);
  EXPECT_FALSE(tmpfile < tmpfile);
}
}    // namespace
}    // namespace tmp
