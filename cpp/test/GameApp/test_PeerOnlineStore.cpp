#include <gtest/gtest.h>
#include "GameApp/CPeerOnlineStore.h"

TEST(PeerOnlineStore_Unit, AddRemoveHasIgnoresZero) {
    CPeerOnlineStore peers;
    peers.Add(0);
    EXPECT_EQ(0u, peers.Size());
    peers.Add(200);
    EXPECT_TRUE(peers.Has(200));
    EXPECT_EQ(1u, peers.Size());
    peers.Remove(200);
    EXPECT_FALSE(peers.Has(200));
    EXPECT_EQ(0u, peers.Size());
}
