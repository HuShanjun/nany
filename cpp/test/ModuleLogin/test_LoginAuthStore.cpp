#include <gtest/gtest.h>
#include "CLoginAuthStore.h"

TEST(LoginAuthStore_Unit, SubmitCompleteFlow) {
    CLoginAuthStore store;
    EXPECT_EQ(0u, store.SubmitAuth(""));
    uint32_t id = store.SubmitAuth("alice");
    ASSERT_NE(0u, id);
    EXPECT_TRUE(store.HasTicket(id));
    EXPECT_TRUE(store.CompleteAuth(id, true));
    EXPECT_FALSE(store.CompleteAuth(999, true));
}
