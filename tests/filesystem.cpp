#ifdef TMP_USE_MODULES
import tmp;
#else
#include <tmp/filesystem>
#endif

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
