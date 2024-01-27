#include <tmp/file.hpp>

#include <gtest/gtest.h>

using tmp::file;
namespace fs = std::filesystem;

TEST(FileTest, CreateFile) {
    {
        const auto tmpfile = file("test");
        ASSERT_TRUE(fs::exists(tmpfile));
    }
    {
        const auto tmpfile = file();
        ASSERT_TRUE(fs::exists(tmpfile));
    }
}

TEST(FileTest, RemoveDirectory) {
    fs::path path;
    {
        const auto tmpfile = file("test");
        path = tmpfile;
        ASSERT_TRUE(fs::exists(path));
    }

    EXPECT_FALSE(fs::exists(path));
}

TEST(FileTest, CreateMultiple) {
    const auto path = "test";

    const auto fst = file(path);
    ASSERT_TRUE(fs::exists(fst));

    const auto snd = file(path);
    ASSERT_TRUE(fs::exists(snd));

    EXPECT_NE(fst.path(), snd.path());
}

TEST(FileTest, MoveConstruction) {
    auto fst = file("test");
    const auto snd = std::move(fst);

    ASSERT_TRUE(fst.path().empty());
    ASSERT_TRUE(fs::exists(snd));
}

TEST(FileTest, MoveAssignment) {
    auto fst = file("test");
    auto snd = file("");

    const auto path1 = fst.path();
    const auto path2 = snd.path();

    fst = std::move(snd);

    ASSERT_FALSE(fs::exists(path1));
    ASSERT_TRUE(fs::exists(path2));

    ASSERT_TRUE(fs::exists(fst));
}

TEST(FileTest, Write) {
    const auto tmpfile = file();
    tmpfile.write("Hello");

    std::ifstream stream(tmpfile.path());
    std::string content(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello");
}

TEST(FileTest, Append) {
    const auto tmpfile = file();
    tmpfile.write("Hello");

    tmpfile.append(", world!");

    std::cout << tmpfile.path() << std::endl;

    std::ifstream stream(tmpfile.path());
    std::string content(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello, world!");
}
