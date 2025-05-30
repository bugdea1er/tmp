#include <tmp/directory>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define UNICODE
#include <Windows.h>
#endif

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Temporary directory prefix for this test suite
constexpr std::string_view prefix = "com.github.bugdea1er.tmp";

/// Tests directory creation with prefix
TEST(directory, create_with_prefix) {
  directory tmpdir = directory(prefix);
  fs::path parent = tmpdir.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::is_directory(tmpdir));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));

#ifdef _WIN32
  constexpr std::wstring_view actual_prefix = L"com.github.bugdea1er.tmp.";
#else
  constexpr std::string_view actual_prefix = "com.github.bugdea1er.tmp.";
#endif

  fs::path::string_type filename = tmpdir.path().filename();
  EXPECT_EQ(filename.substr(0, actual_prefix.length()), actual_prefix);

  fs::perms permissions = fs::status(tmpdir).permissions();
#ifdef _WIN32
  // Following the logic with GetTempFileNameW
  EXPECT_EQ(permissions, fs::perms::all);
#else
  // mkdtemp creates a directory with full access only for the owner
  EXPECT_EQ(permissions, fs::perms::owner_all);
#endif
}

/// Tests directory creation without prefix
TEST(directory, create_without_prefix) {
  directory tmpdir = directory();
  fs::path parent = tmpdir.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::is_directory(tmpdir));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
}

/// Tests multiple directories creation with the same prefix
TEST(directory, create_multiple) {
  directory fst = directory(prefix);
  directory snd = directory(prefix);

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests error handling with invalid prefixes
TEST(directory, create_invalid_prefix) {
  EXPECT_THROW(directory("multi/segment"), std::invalid_argument);
  EXPECT_THROW(directory("/root"), std::invalid_argument);

  fs::path root = fs::temp_directory_path().root_name();
  if (!root.empty()) {
    EXPECT_THROW(directory(root.string() + "relative"), std::invalid_argument);
    EXPECT_THROW(directory(root.string() + "/root"), std::invalid_argument);
  }

#ifdef _WIN32
  EXPECT_THROW(directory("multi\\segment"), std::invalid_argument);
#endif
}

/// Tests creation of a temporary copy of a directory
TEST(directory, copy_directory) {
  directory tmpdir = directory();
  std::ofstream(tmpdir / "file") << "Hello, world!";
  fs::create_directory(tmpdir / "dir");
  std::ofstream(tmpdir / "dir" / "file2") << "Goodbye, world!";

  directory copy = directory::copy(tmpdir);
  EXPECT_TRUE(fs::exists(tmpdir));
  EXPECT_TRUE(fs::exists(copy));
  EXPECT_FALSE(fs::equivalent(tmpdir, copy));

  EXPECT_TRUE(fs::is_directory(copy));

  {
    std::ifstream stream = std::ifstream(copy / "file");
    std::string content = std::string(std::istreambuf_iterator(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }

  {
    std::ifstream stream = std::ifstream(copy / "dir" / "file2");
    std::string content = std::string(std::istreambuf_iterator(stream), {});
    EXPECT_EQ(content, "Goodbye, world!");
  }
}

/// Tests creation of a temporary copy of a file
TEST(directory, copy_file) {
  std::ofstream("existing.txt", std::ios::binary) << "Hello, world!";

  try {
    directory::copy("existing.txt");
    FAIL() << "Expected exception";
  } catch (const fs::filesystem_error& ex) {
    EXPECT_EQ(ex.path1(), "existing.txt");
    EXPECT_EQ(ex.path2(), fs::path());
  }
}

/// Tests creation of a temporary copy without permissions
TEST(directory, copy_directory_without_permissions) {
  directory tmpdir = directory();
  std::ofstream(tmpdir / "file") << "Hello, world!";

#ifdef _WIN32
  HANDLE handle = CreateFile((tmpdir / "file").c_str(), GENERIC_READ, 0, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#else
  fs::permissions(tmpdir / "file", fs::perms::none);
#endif

  try {
    directory::copy(tmpdir);
    FAIL() << "Expected exception";
  } catch (const fs::filesystem_error& ex) {
    EXPECT_EQ(ex.path1(), tmpdir);
    EXPECT_EQ(ex.path2(), fs::path());
  }

#ifdef _WIN32
  CloseHandle(handle);
#else
  fs::permissions(tmpdir / "file", fs::perms::all);
#endif
}

/// Tests `operator/` of directory
TEST(directory, subpath) {
  directory tmpdir = directory();
  fs::path expected = tmpdir.path() / "child";
  std::ofstream(expected) << "Hello, world!";

  EXPECT_TRUE(fs::equivalent(expected, tmpdir / "child"));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / L"child"));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / std::string("child")));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / std::wstring(L"child")));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / std::string_view("child")));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / std::wstring_view(L"child")));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / fs::path("child")));
  EXPECT_TRUE(fs::equivalent(expected, tmpdir / fs::path(L"child")));
}

/// Tests that destructor removes a directory
TEST(directory, destructor) {
  fs::path path;
  {
    directory tmpdir = directory();
    path = tmpdir;
  }

  EXPECT_FALSE(fs::exists(path));
}

/// Tests directory move constructor
TEST(directory, move_constructor) {
  directory fst = directory();
  directory snd = directory(std::move(fst));

  fst.~directory();    // NOLINT(*-use-after-move)

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
}

/// Tests directory move assignment operator
TEST(directory, move_assignment) {
  directory fst = directory();
  {
    directory snd = directory();

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

/// Tests directory swapping
TEST(directory, swap) {
  directory fst = directory();
  directory snd = directory();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
}

/// Tests directory hashing
TEST(directory, hash) {
  directory tmpdir = directory();
  std::hash hash = std::hash<directory>();

  EXPECT_EQ(hash(tmpdir), fs::hash_value(tmpdir.path()));
}
}    // namespace
}    // namespace tmp
