#include <gtest/gtest.h>
#include "GammaNetwork/CAddress.h"

using Gamma::CAddress;
using Gamma::IsIP;
using Gamma::IsPort;

TEST(Address_Unit, ConstructAndGetters)
{
    CAddress addr("127.0.0.1", 8080);
    EXPECT_STREQ("127.0.0.1", addr.GetAddress());
    EXPECT_EQ(8080, addr.GetPort());
}

TEST(Address_Unit, IsIP_IsPort)
{
    EXPECT_TRUE(IsIP("192.168.0.1"));
    EXPECT_FALSE(IsIP("not-an-ip"));
    EXPECT_TRUE(IsPort("443"));
    EXPECT_FALSE(IsPort("70000"));
}
