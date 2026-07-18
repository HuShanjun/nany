#pragma once
#include <gtest/gtest.h>
#include <string>
#include "TestEnv.h"
#include "GammaDatabase/IDatabase.h"

namespace GammaTest {

// GTEST_SKIP() expands to a `return` of the current (void) test function, so
// it cannot be used directly inside a function returning bool. Route it
// through a void lambda so the macro's internal `return` stays local to the
// lambda while this function still reports skip/no-skip to its caller.
inline void EmitSkip(const std::string& reason)
{
    [&]() { GTEST_SKIP() << reason; }();
}

// Returns true if skipped (caller should return immediately after).
inline bool SkipIfNoDb()
{
    using namespace Gamma;
    DbConfig cfg = LoadDbConfigFromEnv();
    IDatabase* db = GetDatabase();
    if (!db) {
        EmitSkip("GetDatabase() returned null");
        return true;
    }
    try {
        IDbConnection* conn = db->CreateConnection(
            cfg.host.c_str(), cfg.port, cfg.user.c_str(), cfg.password.c_str(),
            cfg.database.c_str(), 1, true, false, true);
        if (!conn) {
            EmitSkip("CreateConnection returned null");
            return true;
        }
        conn->Release();
        return false;
    } catch (const std::string& err) {
        EmitSkip("MariaDB unavailable: " + err);
        return true;
    } catch (...) {
        EmitSkip("MariaDB unavailable (unknown error)");
        return true;
    }
}

} // namespace GammaTest
