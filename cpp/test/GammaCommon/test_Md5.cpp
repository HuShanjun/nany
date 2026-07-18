#include <gtest/gtest.h>
#include <cstring>
#include "GammaCommon/GammaMd5.h"

TEST(Md5_Unit, EmptyString_MD5Ex)
{
    uint8_t out[32] = {};
    Gamma::MD5Ex(out, "", 0);
    // MD5("") = d41d8cd98f00b204e9800998ecf8427e (lowercase hex from MD5Ex)
    EXPECT_EQ(0, std::memcmp(out, "d41d8cd98f00b204e9800998ecf8427e", 32));
}

TEST(Md5_Unit, Abc_MD5Ex)
{
    uint8_t out[32] = {};
    const char* msg = "abc";
    Gamma::MD5Ex(out, msg, 3);
    EXPECT_EQ(0, std::memcmp(out, "900150983cd24fb0d6963f7d28e17f72", 32));
}
