#include <gtest/gtest.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include "GammaCommon/CDynamicObject.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaDatabase/IDatabase.h"
#include "GammaMTDbs/IDbsThreadMgr.h"
#include "GammaMTDbs/IDbsQuery.h"
#include "GammaMTDbs/IResultHolder.h"
#include "GammaMTDbs/CBaseDbsConn.h"
#include "RefCountPtr.h"
#include "SkipIf.h"
#include "TestEnv.h"

using namespace Gamma;

// Minimal command protocol: ping the DB thread and echo back "SELECT 1".
enum ETestDbCmd : uint8_t
{
    eTestDbCmd_Ping = 1,
};

#pragma pack(push, 1)
struct SCmdPing
{
    ETestDbCmd eCmd;
};
#pragma pack(pop)

// CBaseDbsConn subclass exercising the shortest DB-thread round trip: no
// table setup, just "SELECT 1" echoed back through the result holder.
class CTestDbsConn : public CBaseDbsConn
{
    DECLARE_DYNAMIC_CLASS(CTestDbsConn);

public:
    void OnConnected(uint32_t /*nThreadIndex*/, IDbConnection** /*aryDbsConn*/,
        uint8_t /*nConnCount*/, bool /*bIsRecord*/) override
    {
    }

    void OnDisConnect(IDbConnection** /*aryDbsConn*/, uint8_t /*nConnCount*/) override
    {
    }

    void OnShellMsg(IDbConnection** aryDbsConn, uint8_t /*nConnCount*/,
        IResultHolder* pRH, const void* pCmd, size_t /*nSize*/) override
    {
        auto eCmd = *(const ETestDbCmd*)pCmd;
        if (eCmd != eTestDbCmd_Ping) {
            return;
        }

        int32_t nValue = 0;
        IDbTextResult* pResult = aryDbsConn[0]->Execute("SELECT 1");
        if (pResult) {
            if (pResult->GetRowNum() > 0) {
                pResult->Locate(0);
                const char* val = pResult->GetData(0);
                nValue = val ? std::atoi(val) : 0;
            }
            pResult->Release();
        }
        pRH->Write(&nValue, sizeof(nValue));
        pRH->Segment();
    }
};

DEFINE_DYNAMIC_CLASS(CTestDbsConn, 2, GET_CLASS_ID(CTestDbsConn));

class CTestDbsQueryHandler : public IDbsQueryHandler
{
public:
    size_t OnQueryResult(const void* pResult, size_t nSize) override
    {
        if (nSize >= sizeof(int32_t)) {
            m_nValue = *(const int32_t*)pResult;
            m_bGotResult = true;
        }
        return nSize;
    }

    bool m_bGotResult = false;
    int32_t m_nValue = 0;
};

TEST(DbsThreadMgr_Integration, CreateAndSimpleQuery)
{
    if (GammaTest::SkipIfNoDb()) {
        return;
    }

    auto cfg = GammaTest::LoadDbConfigFromEnv();

    GammaTest::RefCountPtr<IDbsThreadMgr> mgr(CreateDbsThreadMgr(
        /*nThreadCount*/ 1, CTestDbsConn::GetID(),
        cfg.host.c_str(), cfg.port, cfg.database.c_str(),
        cfg.user.c_str(), cfg.password.c_str(), /*bFoundAsUpdateRow*/ true));
    ASSERT_TRUE(mgr);

    CTestDbsQueryHandler handler;
    GammaTest::RefCountPtr<IDbsQuery> query(mgr->CreateDbsQuery(&handler));
    ASSERT_TRUE(query);

    SCmdPing cmd{ eTestDbCmd_Ping };
    query->Query(/*nChannel*/ 0, &cmd, sizeof(cmd));

    for (int i = 0; i < 50 && !handler.m_bGotResult; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        mgr->Check();
    }

    EXPECT_TRUE(handler.m_bGotResult);
    EXPECT_EQ(handler.m_nValue, 1);
}
