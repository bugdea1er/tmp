#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

namespace tmp {

namespace stdfs = std::filesystem;

/// Tests file creation with prefix
TEST(file, create_with_prefix) {
    file tmpfile = file(PREFIX);
    stdfs::path parent = tmpfile->parent_path();

    EXPECT_TRUE(stdfs::exists(tmpfile));
    EXPECT_TRUE(stdfs::equivalent(parent, stdfs::temp_directory_path() / PREFIX));
}

/// Tests file creation without prefix
TEST(file, create_without_prefix) {
    file tmpfile = file();
    stdfs::path parent = tmpfile->parent_path();

    EXPECT_TRUE(stdfs::exists(tmpfile));
    EXPECT_TRUE(stdfs::equivalent(parent, stdfs::temp_directory_path()));
}

/// Tests multiple file creation with the same prefix
TEST(file, create_multiple) {
    file fst = file(PREFIX);
    file snd = file(PREFIX);

    EXPECT_FALSE(stdfs::equivalent(fst, snd));
}

/// Tests creation of a temporary copy of a file
TEST(file, copy_file) {
    file tmpfile = file(PREFIX);
    tmpfile.write("Hello, world!");

    file tmpcopy = file::copy(tmpfile, PREFIX);
    EXPECT_TRUE(stdfs::exists(tmpfile));
    EXPECT_TRUE(stdfs::exists(tmpcopy));
    EXPECT_FALSE(stdfs::equivalent(tmpfile, tmpcopy));

    EXPECT_EQ(tmpcopy.read(), "Hello, world!");
}

/// Tests creation of a temporary copy of a directory
TEST(file, copy_directory) {
    directory tmpdir = directory(PREFIX);
    EXPECT_THROW(file::copy(tmpdir, PREFIX), stdfs::filesystem_error);
}

/// Tests file reading
TEST(file, read) {
    file tmpfile = file(PREFIX);
    std::ofstream stream = std::ofstream(stdfs::path(tmpfile));

    stream << "Hello," << std::endl;
    stream << "world!" << std::endl;

    EXPECT_EQ(tmpfile.read(), "Hello,\nworld!\n");
}

/// Tests file writing
TEST(file, write) {
    file tmpfile = file(PREFIX);
    tmpfile.write("Hello");

    {
        std::ifstream stream = std::ifstream(stdfs::path(tmpfile));
        auto content = std::string(std::istreambuf_iterator<char>(stream), {});
        EXPECT_EQ(content, "Hello");
    }

    tmpfile.write("world!");

    {
        std::ifstream stream = std::ifstream(stdfs::path(tmpfile));
        auto content = std::string(std::istreambuf_iterator<char>(stream), {});
        EXPECT_EQ(content, "world!");
    }
}

/// Tests file appending
TEST(file, append) {
    file tmpfile = file(PREFIX);
    std::ofstream(stdfs::path(tmpfile)) << "Hello, ";

    tmpfile.append("world");

    {
        std::ifstream stream = std::ifstream(stdfs::path(tmpfile));
        auto content = std::string(std::istreambuf_iterator<char>(stream), {});
        EXPECT_EQ(content, "Hello, world");
    }

    tmpfile.append("!");

    {
        std::ifstream stream = std::ifstream(stdfs::path(tmpfile));
        auto content = std::string(std::istreambuf_iterator<char>(stream), {});
        EXPECT_EQ(content, "Hello, world!");
    }
}

/// Tests that destructor removes a file
TEST(file, destructor) {
    stdfs::path path = stdfs::path();
    {
        file tmpfile = file(PREFIX);
        path = tmpfile;
    }

    EXPECT_FALSE(stdfs::exists(path));
}

/// Tests file move constructor
TEST(file, move_constructor) {
    file fst = file(PREFIX);
    file snd = std::move(fst);

    EXPECT_TRUE(fst->empty());
    EXPECT_TRUE(stdfs::exists(snd));
}

/// Tests file move assignment operator
TEST(file, move_assignment) {
    file fst = file(PREFIX);
    file snd = file(PREFIX);

    stdfs::path path1 = fst;
    stdfs::path path2 = snd;

    fst = std::move(snd);

    EXPECT_FALSE(stdfs::exists(path1));
    EXPECT_TRUE(stdfs::exists(path2));

    EXPECT_TRUE(stdfs::exists(fst));
    EXPECT_TRUE(stdfs::equivalent(fst, path2));
}

/// Tests file releasing
TEST(file, release) {
    stdfs::path path = stdfs::path();
    {
        file tmpfile = file(PREFIX);
        stdfs::path expected = stdfs::path(tmpfile);
        path = tmpfile.release();

        EXPECT_TRUE(stdfs::equivalent(path, expected));
    }

    EXPECT_TRUE(stdfs::exists(path));
    stdfs::remove(path);
}

/// Tests file moving
TEST(file, move) {
    stdfs::path path = stdfs::path();
    stdfs::path to = stdfs::temp_directory_path() / PREFIX / "non-existing/parent";
    {
        file tmpfile = file(PREFIX);
        path = tmpfile;

        tmpfile.move(to);
    }

    EXPECT_FALSE(stdfs::exists(path));
    EXPECT_TRUE(stdfs::exists(to));
    stdfs::remove_all(stdfs::temp_directory_path() / PREFIX / "non-existing");
}
}    // namespace tmp
