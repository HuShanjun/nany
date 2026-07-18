#include <gtest/gtest.h>
#include <fstream>
#include "TempDir.h"

TEST(TempDir_Unit, CreateAndCleanup)
{
    namespace fs = std::filesystem;
    fs::path kept;
    {
        GammaTest::TempDir dir;
        kept = dir.path();
        ASSERT_TRUE(fs::exists(kept));
        std::ofstream(kept / "marker.txt") << "ok";
        ASSERT_TRUE(fs::exists(kept / "marker.txt"));
    }
    EXPECT_FALSE(fs::exists(kept));
}
