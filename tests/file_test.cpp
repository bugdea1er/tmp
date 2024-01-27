#include <tmp/file.hpp>

#include <gtest/gtest.h>

namespace fs = std::filesystem;
static auto prefix = "org.github.bugdea1er.tmp";

TEST(FileTest, CreateFile) {
    {
        const auto tmpfile = tmp::file(prefix);
        ASSERT_TRUE(fs::exists(tmpfile));
    }
    {
        const auto tmpfile = tmp::file();
        ASSERT_TRUE(fs::exists(tmpfile));
    }
}

TEST(FileTest, RemoveDirectory) {
    auto path = fs::path();
    {
        const auto tmpfile = tmp::file(prefix);
        path = tmpfile;
        ASSERT_TRUE(fs::exists(path));
    }

    EXPECT_FALSE(fs::exists(path));
}

TEST(FileTest, CreateMultiple) {
    const auto fst = tmp::file(prefix);
    ASSERT_TRUE(fs::exists(fst));

    const auto snd = tmp::file(prefix);
    ASSERT_TRUE(fs::exists(snd));

    EXPECT_NE(fs::path(fst), snd);
}

TEST(FileTest, MoveConstruction) {
    auto fst = tmp::file(prefix);
    const auto snd = std::move(fst);

    ASSERT_TRUE(fst->empty());
    ASSERT_TRUE(fs::exists(snd));
}

TEST(FileTest, MoveAssignment) {
    auto fst = tmp::file(prefix);
    auto snd = tmp::file(prefix);

    const auto path1 = fs::path(fst);
    const auto path2 = fs::path(snd);

    fst = std::move(snd);

    ASSERT_FALSE(fs::exists(path1));
    ASSERT_TRUE(fs::exists(path2));

    ASSERT_TRUE(fs::exists(fst));
    ASSERT_EQ(fs::path(fst), path2);
}

TEST(FileTest, Write) {
    const auto tmpfile = tmp::file(prefix);
    tmpfile.write("Hello");

    auto stream = std::ifstream(fs::path(tmpfile));
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello");
}

TEST(FileTest, Append) {
    const auto tmpfile = tmp::file(prefix);

    tmpfile.write("Hello");
    tmpfile.append(", world!");

    auto stream = std::ifstream(fs::path(tmpfile));
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello, world!");
}
