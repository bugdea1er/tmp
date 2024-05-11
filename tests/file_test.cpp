#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

namespace tmp {

namespace fs = std::filesystem;

TEST(FileTest, CreateFile) {
    {
        const auto tmpfile = file(PREFIX);
        const auto parent = tmpfile->parent_path();

        ASSERT_TRUE(fs::exists(tmpfile));
        ASSERT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / PREFIX));
    }
    {
        const auto tmpfile = file();
        const auto parent = tmpfile->parent_path();

        ASSERT_TRUE(fs::exists(tmpfile));
        ASSERT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
    }
}

TEST(FileTest, RemoveFile) {
    auto path = fs::path();
    {
        const auto tmpfile = file(PREFIX);
        path = tmpfile;
        ASSERT_TRUE(fs::exists(path));
    }

    EXPECT_FALSE(fs::exists(path));
}

TEST(FileTest, CreateMultiple) {
    const auto fst = file(PREFIX);
    ASSERT_TRUE(fs::exists(fst));

    const auto snd = file(PREFIX);
    ASSERT_TRUE(fs::exists(snd));

    EXPECT_NE(fs::path(fst), fs::path(snd));
}

TEST(FileTest, Release) {
    auto path = fs::path();
    {
        auto tmpfile = file(PREFIX);
        auto expected = fs::path(tmpfile);
        path = tmpfile.release();
        ASSERT_EQ(path, expected);
        ASSERT_TRUE(fs::exists(path));
    }

    ASSERT_TRUE(fs::exists(path));
    fs::remove(path);
}

TEST(FileTest, MoveFile) {
    auto path = fs::path();
    auto to = fs::temp_directory_path() / PREFIX / "non-existing" / "parent";
    {
        auto tmpfile = file(PREFIX);
        path = tmpfile;

        tmpfile.move(to);
    }

    ASSERT_FALSE(fs::exists(path));
    ASSERT_TRUE(fs::exists(to));
    fs::remove_all(fs::temp_directory_path() / PREFIX / "non-existing");
}

TEST(FileTest, MoveConstruction) {
    auto fst = file(PREFIX);
    const auto snd = std::move(fst);

    ASSERT_TRUE(fst->empty());
    ASSERT_TRUE(fs::exists(snd));
}

TEST(FileTest, MoveAssignment) {
    auto fst = file(PREFIX);
    auto snd = file(PREFIX);

    const auto path1 = fs::path(fst);
    const auto path2 = fs::path(snd);

    fst = std::move(snd);

    ASSERT_FALSE(fs::exists(path1));
    ASSERT_TRUE(fs::exists(path2));

    ASSERT_TRUE(fs::exists(fst));
    ASSERT_EQ(fs::path(fst), path2);
}

TEST(FileTest, Read) {
    const auto tmpfile = file(PREFIX);
    tmpfile.write("Hello");
    tmpfile.append(", world!");

    auto content = tmpfile.read();
    ASSERT_EQ(content, "Hello, world!");
}

TEST(FileTest, Write) {
    const auto tmpfile = file(PREFIX);
    tmpfile.write("Hello");

    auto stream = std::ifstream(fs::path(tmpfile));
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello");
}

TEST(FileTest, Append) {
    const auto tmpfile = file(PREFIX);

    tmpfile.write("Hello");
    tmpfile.append(", world!");

    auto stream = std::ifstream(fs::path(tmpfile));
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    ASSERT_EQ(content, "Hello, world!");
}

TEST(FileTest, Execute) {
    const auto tmpfile = file(PREFIX);

    tmpfile.write("#!/bin/sh\n");
    tmpfile.append("echo $0\n");
    tmpfile.append("echo $1\n");

    ASSERT_EQ(0, tmpfile.execute({"1"}));
}

TEST(FileTest, Copy) {
    {
        const auto tmpfile = file(PREFIX);
        tmpfile.write("Hello, world!");

        const auto tmpcopy = file::copy(tmpfile, PREFIX);
        ASSERT_EQ(tmpcopy.read(), "Hello, world!");
        EXPECT_NE(fs::path(tmpfile), fs::path(tmpcopy));
    }
    {
        const auto tmpdir = directory(PREFIX);
        ASSERT_THROW(file::copy(tmpdir, PREFIX), fs::filesystem_error);
    }
}
}    // namespace tmp
