#include <tmp/file>
#include <tmp/fs>

#include <gtest/gtest.h>

namespace tmp {

TEST(fs, root) {
    EXPECT_NO_THROW(fs::root());
    EXPECT_NO_THROW(fs::root(PREFIX));
}

TEST(fs, space) {
    EXPECT_NO_THROW(fs::space());
}
}    // namespace tmp
