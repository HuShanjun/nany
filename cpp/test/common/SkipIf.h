#pragma once
#include <gtest/gtest.h>
#include <string>
#include "TestEnv.h"
#include "GammaDatabase/IDatabase.h"

namespace GammaTest {

// Returns true if skipped (caller should return immediately after).
inline bool SkipIfNoDb()
{
    using namespace Gamma;
    DbConfig cfg = LoadDbConfigFromEnv();
    IDatabase* db = GetDatabase();
    if (!db) {
        GTEST_SKIP() << "GetDatabase() returned null";
        return true;
    }
    try {
        IDbConnection* conn = db->CreateConnection(
            cfg.host.c_str(), cfg.port, cfg.user.c_str(), cfg.password.c_str(),
            cfg.database.c_str(), 1, true, false, true);
        if (!conn) {
            GTEST_SKIP() << "CreateConnection returned null";
            return true;
        }
        conn->Release();
        return false;
    } catch (const std::string& err) {
        GTEST_SKIP() << "MariaDB unavailable: " << err;
        return true;
    } catch (...) {
        GTEST_SKIP() << "MariaDB unavailable (unknown error)";
        return true;
    }
}

} // namespace GammaTest
