#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <ostream>
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
bool native_handle_is_valid(file::native_handle_type handle) {
#ifdef WIN32
  BY_HANDLE_FILE_INFORMATION info;
  return GetFileInformationByHandle(handle, &info);
#else
  return fcntl(handle, F_GETFD) != -1;
#endif
}
}    // namespace

/// Tests file creation with prefix
TEST(file, create_with_prefix) {
  file tmpfile = file(PREFIX);
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / PREFIX));
  EXPECT_TRUE(native_handle_is_valid(tmpfile.native_handle()));

  fs::perms permissions = fs::status(tmpfile).permissions();
#ifdef WIN32
  // GetTempFileNameW creates a file with all permissions
  EXPECT_EQ(permissions, fs::perms::all);
#else
  // mkstemp creates a file that can only be read and written by the owner
  EXPECT_EQ(permissions, fs::perms::owner_read | fs::perms::owner_write);
#endif
}

/// Tests file creation without prefix
TEST(file, create_without_prefix) {
  file tmpfile = file();
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
  EXPECT_TRUE(native_handle_is_valid(tmpfile.native_handle()));
}

/// Tests file creation with suffix
TEST(file, create_with_suffix) {
  file tmpfile = file("", ".test");

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_EQ(tmpfile.path().extension(), ".test");
  EXPECT_TRUE(native_handle_is_valid(tmpfile.native_handle()));
}

/// Tests multiple file creation with the same prefix
TEST(file, create_multiple) {
  file fst = file(PREFIX);
  file snd = file(PREFIX);

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests creation of a temporary copy of a file
TEST(file, copy_file) {
  file tmpfile = file();
  tmpfile.write("Hello, world!");

  file tmpcopy = file::copy(tmpfile);
  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::exists(tmpcopy));
  EXPECT_FALSE(fs::equivalent(tmpfile, tmpcopy));

  EXPECT_TRUE(fs::is_regular_file(tmpfile));

  EXPECT_EQ(tmpcopy.read(), "Hello, world!");
}

/// Tests creation of a temporary copy of a directory
TEST(file, copy_directory) {
  directory tmpdir = directory();
  EXPECT_THROW(file::copy(tmpdir), fs::filesystem_error);
}

/// Tests binary file reading
TEST(file, read_binary) {
  file tmpfile = file();
  std::ofstream stream = std::ofstream(tmpfile.path(), std::ios::binary);

  stream << "Hello," << std::endl;
  stream << "world!" << std::endl;

  EXPECT_EQ(tmpfile.read(), "Hello,\nworld!\n");
}

/// Tests text file reading
TEST(file, read_text) {
  file tmpfile = file::text();
  std::ofstream stream = std::ofstream(tmpfile.path());

  stream << "Hello," << std::endl;
  stream << "world!" << std::endl;

  EXPECT_EQ(tmpfile.read(), "Hello,\nworld!\n");
}

/// Tests binary file writing
TEST(file, write_binary) {
  file tmpfile = file();
  tmpfile.write("Hello\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello\n");
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "world!\n");
  }
}

/// Tests text file writing
TEST(file, write_text) {
  file tmpfile = file::text();
  tmpfile.write("Hello\n");

  {
    auto stream = std::ifstream(tmpfile.path());
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello\n");
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path());
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "world!\n");
  }
}

/// Tests binary file appending
TEST(file, append_binary) {
  file tmpfile = file();
  std::ofstream(tmpfile.path(), std::ios::binary) << "Hello, ";

  tmpfile.append("world");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world");
  }

  tmpfile.append("!");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }
}

/// Tests text file appending
TEST(file, append_text) {
  file tmpfile = file::text();
  std::ofstream(tmpfile.path()) << "Hello,\n ";

  tmpfile.append("world");

  {
    auto stream = std::ifstream(tmpfile.path());
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello,\n world");
  }

  tmpfile.append("!");

  {
    auto stream = std::ifstream(tmpfile.path());
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello,\n world!");
  }
}

/// Tests that destructor removes a file
TEST(file, destructor) {
  fs::path path = fs::path();
  file::native_handle_type handle;
  {
    file tmpfile = file();
    path = tmpfile;
    handle = tmpfile.native_handle();
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));
}

/// Tests file move constructor
TEST(file, move_constructor) {
  file fst = file();
  file snd = std::move(fst);

  EXPECT_TRUE(fst.path().empty());
  EXPECT_TRUE(fs::exists(snd));
  EXPECT_TRUE(native_handle_is_valid(snd.native_handle()));
}

/// Tests file move assignment operator
TEST(file, move_assignment) {
  file fst = file();
  file snd = file();

  fs::path path1 = fst;
  fs::path path2 = snd;

  file::native_handle_type fst_handle = fst.native_handle();
  file::native_handle_type snd_handle = snd.native_handle();

  fst = std::move(snd);

  EXPECT_FALSE(fs::exists(path1));
  EXPECT_TRUE(fs::exists(path2));

  EXPECT_TRUE(fs::exists(fst));
  EXPECT_TRUE(fs::equivalent(fst, path2));

  EXPECT_FALSE(native_handle_is_valid(fst_handle));
  EXPECT_TRUE(native_handle_is_valid(snd_handle));
}

/// Tests file moving
TEST(file, move) {
  fs::path path = fs::path();
  file::native_handle_type handle;

  fs::path to = fs::temp_directory_path() / "non-existing" / "parent";
  {
    file tmpfile = file();
    path = tmpfile;
    handle = tmpfile.native_handle();

    tmpfile.move(to);
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_TRUE(fs::exists(to));
  EXPECT_FALSE(native_handle_is_valid(handle));

  fs::remove_all(fs::temp_directory_path() / "non-existing");
}
}    // namespace tmp
