#include <tmp/directory>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

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
}    // namespace
}    // namespace tmp
