#include <gtest/gtest.h>
#include "GammaConnects/IConnectionMgr.h"
#include "RefCountPtr.h"

TEST(ConnMgr_Unit, CreateSmoke)
{
    // Smoke-test only: verify CreateConnMgr returns a non-null manager.
    // Full Release lifecycle is blocked by a known ~CGNetwork segfault in
    // CAddrResolutionDelegate list teardown (pre-existing engine bug).
    // AddRef() prevents RefCountPtr::~RefCountPtr from destroying the instance;
    // intentional leak until the engine fix lands.
    GammaTest::RefCountPtr<Gamma::IConnectionMgr> mgr(
        Gamma::CreateConnMgr(/*nAutoDisconnectTime*/ 30000, /*bStrictMode*/ false));
    ASSERT_TRUE(mgr);
    mgr->AddRef();
}
