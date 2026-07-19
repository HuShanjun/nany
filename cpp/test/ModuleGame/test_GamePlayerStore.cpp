#include <gtest/gtest.h>
#include "CGamePlayerStore.h"

TEST(GamePlayerStore_Unit, EnterRequiresPeerAndCleanup) {
    CGamePlayerStore store;
    EXPECT_FALSE(store.PlayerEnter(1001, 300));
    store.AddPeer(300);
    EXPECT_TRUE(store.PlayerEnter(1001, 300));
    EXPECT_TRUE(store.HasPlayer(1001));
    store.RemovePeer(300);
    EXPECT_FALSE(store.HasPlayer(1001));
}
