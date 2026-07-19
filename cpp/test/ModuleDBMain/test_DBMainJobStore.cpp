#include <gtest/gtest.h>
#include "CDBMainJobStore.h"

TEST(DBMainJobStore_Unit, EnqueueCompleteFlow) {
    CDBMainJobStore store;
    EXPECT_EQ(0u, store.Enqueue(100));
    store.AddPeer(100);
    uint32_t job = store.Enqueue(100);
    ASSERT_NE(0u, job);
    EXPECT_TRUE(store.HasJob(job));
    EXPECT_TRUE(store.Complete(job));
    EXPECT_FALSE(store.Complete(999));
}
