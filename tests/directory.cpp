#include <tmp/directory>
#include <tmp/file>

#include <gtest/gtest.h>

namespace tmp {

namespace stdfs = std::filesystem;

/// Tests directory creation with prefix
TEST(directory, create_with_prefix) {
    directory tmpdir = directory(PREFIX);
    stdfs::path parent = tmpdir->parent_path();

    EXPECT_TRUE(stdfs::exists(tmpdir));
    EXPECT_TRUE(stdfs::equivalent(parent, stdfs::temp_directory_path() / PREFIX));
}

/// Tests directory creation without prefix
TEST(directory, create_without_prefix) {
    directory tmpdir = directory();
    stdfs::path parent = tmpdir->parent_path();

    EXPECT_TRUE(stdfs::exists(tmpdir));
    EXPECT_TRUE(stdfs::equivalent(parent, stdfs::temp_directory_path()));
}

/// Tests multiple directories creation with the same prefix
TEST(directory, create_multiple) {
    directory fst = directory(PREFIX);
    directory snd = directory(PREFIX);

    EXPECT_FALSE(stdfs::equivalent(fst, snd));
}

/// Tests creation of a temporary copy of a directory
TEST(directory, copy_directory) {
    directory tmpdir = directory(PREFIX);
    std::ofstream(tmpdir / "file") << "Hello, world!";

    directory tmpcopy = directory::copy(tmpdir, PREFIX);
    EXPECT_TRUE(stdfs::exists(tmpdir));
    EXPECT_TRUE(stdfs::exists(tmpcopy));
    EXPECT_FALSE(stdfs::equivalent(tmpdir, tmpcopy));

    std::ifstream stream = std::ifstream(tmpcopy / "file");
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world!");
}

/// Tests creation of a temporary copy of a file
TEST(directory, copy_file) {
    file tmpfile = file(PREFIX);
    EXPECT_THROW(directory::copy(tmpfile, PREFIX), stdfs::filesystem_error);
}

/// Tests `operator/` of directory
TEST(directory, subpath) {
    directory tmpdir = directory(PREFIX);
    stdfs::path child = tmpdir / "child";

    EXPECT_TRUE(stdfs::equivalent(tmpdir, child.parent_path()));
}

/// Tests that destructor removes a directory
TEST(directory, destructor) {
    stdfs::path path = stdfs::path();
    {
        directory tmpdir = directory(PREFIX);
        path = tmpdir;
    }

    EXPECT_FALSE(stdfs::exists(path));
}

/// Tests directory move constructor
TEST(directory, move_constructor) {
    directory fst = directory(PREFIX);
    directory snd = std::move(fst);

    EXPECT_TRUE(fst->empty());
    EXPECT_TRUE(stdfs::exists(snd));
}

/// Tests directory move assignment operator
TEST(directory, move_assignment) {
    directory fst = directory(PREFIX);
    directory snd = directory(PREFIX);

    stdfs::path path1 = fst;
    stdfs::path path2 = snd;

    fst = std::move(snd);

    EXPECT_FALSE(stdfs::exists(path1));
    EXPECT_TRUE(stdfs::exists(path2));

    EXPECT_TRUE(stdfs::exists(fst));
    EXPECT_TRUE(stdfs::equivalent(fst, path2));
}

/// Tests directory releasing
TEST(directory, release) {
    stdfs::path path = stdfs::path();
    {
        directory tmpdir = directory(PREFIX);
        stdfs::path expected = tmpdir;
        path = tmpdir.release();

        EXPECT_TRUE(stdfs::equivalent(path, expected));
    }

    EXPECT_TRUE(stdfs::exists(path));
    stdfs::remove(path);
}

/// Tests directory moving
TEST(directory, move) {
    stdfs::path path = stdfs::path();
    stdfs::path to = stdfs::temp_directory_path() / PREFIX / "non-existing/parent";
    {
        directory tmpdir = directory(PREFIX);
        path = tmpdir;

        tmpdir.move(to);
    }

    EXPECT_FALSE(stdfs::exists(path));
    EXPECT_TRUE(stdfs::exists(to));
    stdfs::remove_all(stdfs::temp_directory_path() / PREFIX / "non-existing");
}
}    // namespace tmp
