#include <gtest/gtest.h>
#include "CGateSessionStore.h"

TEST(GateSessionStore_Unit, AddRemoveClient) {
    CGateSessionStore store;
    store.AddClient(1, 100);
    ASSERT_TRUE(store.HasClient(1));
    auto* s = store.FindClient(1);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(1u, s->nClientID);
    EXPECT_EQ(0u, s->nBoundServerID);
    EXPECT_EQ(100, s->nConnectTime);
    store.RemoveClient(1);
    EXPECT_FALSE(store.HasClient(1));
}

TEST(GateSessionStore_Unit, BindRequiresClientAndServer) {
    CGateSessionStore store;
    EXPECT_FALSE(store.BindClientToServer(1, 200));
    store.AddClient(1, 0);
    EXPECT_FALSE(store.BindClientToServer(1, 200));
    store.AddServer(200);
    EXPECT_TRUE(store.BindClientToServer(1, 200));
    EXPECT_EQ(200u, store.FindClient(1)->nBoundServerID);
}

TEST(GateSessionStore_Unit, RemoveServerClearsBindings) {
    CGateSessionStore store;
    store.AddClient(1, 0);
    store.AddClient(2, 0);
    store.AddServer(200);
    store.BindClientToServer(1, 200);
    store.BindClientToServer(2, 200);
    store.RemoveServer(200);
    EXPECT_FALSE(store.HasServer(200));
    EXPECT_EQ(0u, store.FindClient(1)->nBoundServerID);
    EXPECT_EQ(0u, store.FindClient(2)->nBoundServerID);
}

TEST(GateSessionStore_Unit, CanForwardChecks) {
    CGateSessionStore store;
    EXPECT_FALSE(store.CanForwardToClient(1));
    EXPECT_FALSE(store.CanForwardToServer(1, 200));
    store.AddClient(1, 0);
    store.AddServer(200);
    EXPECT_TRUE(store.CanForwardToClient(1));
    EXPECT_TRUE(store.CanForwardToServer(1, 200));
}
