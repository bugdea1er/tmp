#include <tmp/directory.hpp>

#include <gtest/gtest.h>

using tmp::directory;
namespace fs = std::filesystem;

TEST(DirectoryTest, CreateDirectory) {
    const auto tmpdir = directory("test");
    ASSERT_TRUE(fs::exists(tmpdir));
}

TEST(DirectoryTest, RemoveDirectory) {
    fs::path path;
    {
        const auto tmpdir = directory("test");
        path = tmpdir;
        ASSERT_TRUE(fs::exists(path));
    }

    EXPECT_FALSE(fs::exists(path));
}

TEST(DirectoryTest, CreateMultiple) {
    const auto path = "test";

    const auto fst = directory(path);
    ASSERT_TRUE(fs::exists(fst));

    const auto snd = directory(path);
    ASSERT_TRUE(fs::exists(snd));

    EXPECT_NE(fst.path(), snd.path());
}

TEST(DirectoryTest, SubpathTest) {
    const auto tmpdir = directory("test");
    const auto child = tmpdir / "child";

    ASSERT_EQ(tmpdir, child.parent_path());
}

TEST(DirectoryTest, MoveConstruction) {
    auto fst = directory("test");
    const auto snd = std::move(fst);

    ASSERT_TRUE(fst.path().empty());
    ASSERT_TRUE(fs::exists(snd));
}

TEST(DirectoryTest, MoveAssignment) {
    auto fst = directory("test");
    auto snd = directory("");

    const auto path1 = fst.path();
    const auto path2 = snd.path();

    fst = std::move(snd);

    ASSERT_FALSE(fs::exists(path1));
    ASSERT_TRUE(fs::exists(path2));

    ASSERT_TRUE(fs::exists(fst));
}
