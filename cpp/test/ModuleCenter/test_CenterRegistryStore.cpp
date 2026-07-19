#include <gtest/gtest.h>
#include "CCenterRegistryStore.h"

TEST(CenterRegistryStore_Unit, RegisterRequiresPeer) {
    CCenterRegistryStore store;
    EXPECT_FALSE(store.RegisterServer(200, 2));
    store.AddPeer(200);
    EXPECT_TRUE(store.RegisterServer(200, 2));
    EXPECT_TRUE(store.IsRegistered(200));
    store.RemovePeer(200);
    EXPECT_FALSE(store.IsRegistered(200));
    EXPECT_FALSE(store.HasPeer(200));
}
