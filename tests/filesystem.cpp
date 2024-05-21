#include <tmp/filesystem>

#include <gtest/gtest.h>

namespace tmp {

TEST(filesystem, root) {
    EXPECT_NO_THROW(filesystem::root());
    EXPECT_NO_THROW(filesystem::root(PREFIX));
}

TEST(filesystem, space) {
    EXPECT_NO_THROW(filesystem::space());
}
}    // namespace tmp
