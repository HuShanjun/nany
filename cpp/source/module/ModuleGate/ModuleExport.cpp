#include "ModuleGate.h"
#include "Interface/IGameApp.h"

#if defined(_WIN32)
#define MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define MODULE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static IConsummerPtr g_pConsummer;

MODULE_EXPORT void InitDllServant(IGameApp* /*pApp*/) {
    // GameApp already SetGameApp; pApp retained for future use.
}

MODULE_EXPORT void GetConsummer(IConsummerPtr& out) {
    if (!g_pConsummer) {
        g_pConsummer = new CModuleGate();
    }
    out = g_pConsummer;
}
