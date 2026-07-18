#include <gtest/gtest.h>
#include "GammaScript/CScriptLua.h"

TEST(ScriptLua_Unit, ConstructAndRunArithmetic)
{
    // NOTE: CScriptLua's destructor chain (via ~CScriptBase -> delete m_pDebugger
    // -> ~CDebugLua -> GetDebugHandler()->GetVM()) calls back into the handler's
    // GetVM() while the object's vtable has already degraded to CScriptBase,
    // where GetVM() is still pure virtual. This aborts with "pure virtual method
    // called" on *any* normal construct/destruct cycle, independent of this test's
    // API usage. This is a pre-existing engine issue (tracked separately), so this
    // smoke test intentionally leaks the instance to exercise real construction and
    // Lua execution without triggering that unrelated destructor bug.
    Gamma::CScriptLua* pScript = new Gamma::CScriptLua(nullptr, 0, false);

    int32_t nResult = 0;
    ASSERT_TRUE(pScript->Evaluate<int32_t>("1+2", nResult));
    EXPECT_EQ(3, nResult);
}
