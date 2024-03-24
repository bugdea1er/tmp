#include <tmp/directory>

#include <gtest/gtest.h>
#include <tmp/file>

namespace fs = tmp::fs;

TEST(DirectoryTest, CreateDirectory) {
    {
        const auto tmpdir = tmp::directory(PREFIX);
        const auto parent = tmpdir->parent_path();

        ASSERT_TRUE(fs::exists(tmpdir));
        ASSERT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / PREFIX));
    }
    {
        const auto tmpdir = tmp::directory();
        const auto parent = tmpdir->parent_path();

        ASSERT_TRUE(fs::exists(tmpdir));
        ASSERT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
    }
}

TEST(DirectoryTest, RemoveDirectory) {
    auto path = fs::path();
    {
        const auto tmpdir = tmp::directory(PREFIX);
        path = tmpdir;
        ASSERT_TRUE(fs::exists(path));
    }

    EXPECT_FALSE(fs::exists(path));
}

TEST(DirectoryTest, CreateMultiple) {
    const auto fst = tmp::directory(PREFIX);
    ASSERT_TRUE(fs::exists(fst));

    const auto snd = tmp::directory(PREFIX);
    ASSERT_TRUE(fs::exists(snd));

    EXPECT_NE(fs::path(fst), fs::path(snd));
}

TEST(DirectoryTest, SubpathTest) {
    const auto tmpdir = tmp::directory(PREFIX);
    const auto child = tmpdir / "child";

    ASSERT_EQ(fs::path(tmpdir), child.parent_path());
}

TEST(DirectoryTest, Release) {
    auto path = fs::path();
    {
        auto tmpdir = tmp::directory(PREFIX);
        auto expected = fs::path(tmpdir);
        path = tmpdir.release();
        ASSERT_EQ(path, expected);
        ASSERT_TRUE(fs::exists(path));
    }

    ASSERT_TRUE(fs::exists(path));
    fs::remove(path);
}

TEST(DirectoryTest, MoveDirectory) {
    auto path = fs::path();
    auto to = fs::temp_directory_path() / PREFIX / "moved";
    {
        auto tmpdir = tmp::directory(PREFIX);
        path = tmpdir;

        tmpdir.move(to);
    }

    ASSERT_FALSE(fs::exists(path));
    ASSERT_TRUE(fs::exists(to));
    fs::remove_all(to);
}

TEST(DirectoryTest, MoveConstruction) {
    auto fst = tmp::directory(PREFIX);
    const auto snd = std::move(fst);

    ASSERT_TRUE(fst->empty());
    ASSERT_TRUE(fs::exists(snd));
}

TEST(DirectoryTest, MoveAssignment) {
    auto fst = tmp::directory(PREFIX);
    auto snd = tmp::directory(PREFIX);

    const auto path1 = fs::path(fst);
    const auto path2 = fs::path(snd);

    fst = std::move(snd);

    ASSERT_FALSE(fs::exists(path1));
    ASSERT_TRUE(fs::exists(path2));

    ASSERT_TRUE(fs::exists(fst));
    ASSERT_EQ(fs::path(fst), path2);
}

TEST(DirectoryTest, Copy) {
    {
        const auto tmpdir = tmp::directory(PREFIX);

        auto file = std::ofstream(tmpdir / "file");
        file << "Hello, world!";
        file.close();

        const auto tmpcopy = tmp::directory::copy(tmpdir, PREFIX);

        auto stream = std::ifstream(tmpcopy / "file");
        auto content = std::string(std::istreambuf_iterator<char>(stream), {});
        ASSERT_EQ(content, "Hello, world!");

        EXPECT_NE(fs::path(tmpdir), fs::path(tmpcopy));
    }
    {
        const auto tmpfile = tmp::file(PREFIX);
        ASSERT_THROW(tmp::directory::copy(tmpfile, PREFIX), fs::filesystem_error);
    }
}
