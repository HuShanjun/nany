#include <gtest/gtest.h>
#include "GammaDatabase/IDatabase.h"
#include "SkipIf.h"
#include "TestEnv.h"

TEST(Database_Integration, ConnectAndSimpleQuery)
{
    if (GammaTest::SkipIfNoDb()) {
        return;
    }
    auto cfg = GammaTest::LoadDbConfigFromEnv();
    Gamma::IDatabase* db = Gamma::GetDatabase();
    Gamma::IDbConnection* conn = db->CreateConnection(
        cfg.host.c_str(), cfg.port, cfg.user.c_str(), cfg.password.c_str(),
        cfg.database.c_str(), 1, true, false, true);
    ASSERT_NE(conn, nullptr);

    Gamma::IDbTextResult* result = conn->Execute("SELECT 1 AS one");
    ASSERT_NE(result, nullptr);
    ASSERT_GE(result->GetRowNum(), 1u);
    result->Locate(0);
    const char* val = result->GetData(0);
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ("1", val);
    result->Release();
    conn->Release();
}
