#include <tmp/file.hpp>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

TEST(FileTest, CreateFile) {
    {
        const auto tmpfile = tmp::file("test");
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
        const auto tmpfile = tmp::file("test");
        path = tmpfile;
        ASSERT_TRUE(fs::exists(path));
    }

    EXPECT_FALSE(fs::exists(path));
}

TEST(FileTest, CreateMultiple) {
    const auto path = "test";

    const auto fst = tmp::file(path);
    ASSERT_TRUE(fs::exists(fst));

    const auto snd = tmp::file(path);
    ASSERT_TRUE(fs::exists(snd));

    EXPECT_NE(fst, snd);
}

TEST(FileTest, MoveConstruction) {
    auto fst = tmp::file("test");
    const auto snd = std::move(fst);

    ASSERT_TRUE(fst->empty());
    ASSERT_TRUE(fs::exists(snd));
}

TEST(FileTest, MoveAssignment) {
    auto fst = tmp::file("test");
    auto snd = tmp::file("");

    const auto path1 = fs::path(fst);
    const auto path2 = fs::path(snd);

    fst = std::move(snd);

    ASSERT_FALSE(fs::exists(path1));
    ASSERT_TRUE(fs::exists(path2));

    ASSERT_TRUE(fs::exists(fst));
}

TEST(FileTest, Write) {
    const auto tmpfile = tmp::file();
    tmpfile.write("Hello");

    std::ifstream stream(tmpfile);
    std::string content(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello");
}

TEST(FileTest, Append) {
    const auto tmpfile = tmp::file();

    tmpfile.write("Hello");
    tmpfile.append(", world!");

    std::ifstream stream(tmpfile);
    std::string content(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello, world!");
}
