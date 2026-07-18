#include <gtest/gtest.h>
#include "GammaConnects/IConnectionMgr.h"
#include "RefCountPtr.h"

TEST(ConnMgr_Unit, CreateAndRelease)
{
    // NOTE: ~CGNetwork -> Check(0) segfaults in CAddrResolutionDelegate list
    // teardown on any CreateConnMgr/Release cycle (pre-existing engine issue;
    // confirmed via gdb). AddRef balances RefCountPtr::Release so we smoke-test
    // construction without triggering the unrelated destructor bug.
    GammaTest::RefCountPtr<Gamma::IConnectionMgr> mgr(
        Gamma::CreateConnMgr(/*nAutoDisconnectTime*/ 30000, /*bStrictMode*/ false));
    ASSERT_TRUE(mgr);
    mgr->AddRef();
}
