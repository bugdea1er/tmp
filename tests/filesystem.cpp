#include <tmp/file>
#include <tmp/filesystem>

#include <gtest/gtest.h>

namespace tmp {

TEST(fs, root) {
    EXPECT_NO_THROW(filesystem::root());
    EXPECT_NO_THROW(filesystem::root(PREFIX));
}

TEST(fs, space) {
    EXPECT_NO_THROW(filesystem::space());
}
}    // namespace tmp
