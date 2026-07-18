#include <gtest/gtest.h>
#include "toml++/toml.hpp"
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static fs::path ProcessModuleTomlPath() {
    // Prefer source-tree config relative to binary or CMAKE define; fall back to known repo path.
    const char* szEnv = std::getenv("NANY_PROCESS_MODULE_TOML");
    if (szEnv && *szEnv) {
        return fs::path(szEnv);
    }
    return fs::path(NANY_SOURCE_DIR) / "data/devops/etc/process_module.toml";
}

TEST(ProcessModuleToml_Unit, AllServerTypesHaveMainModule) {
    auto path = ProcessModuleTomlPath();
    ASSERT_TRUE(fs::exists(path)) << path;
    toml::table tbl = toml::parse_file(path.string());

    struct Expect {
        const char* section;
        const char* moduleName;
        int64_t moduleId;
    };
    const Expect expects[] = {
        {"Login", "ModuleLogin", 10000},
        {"Game", "ModuleGame", 20000},
        {"Center", "ModuleCenter", 30000},
        {"Gate", "ModuleGate", 40000},
        {"DBMain", "ModuleDBMain", 50000},
    };

    for (const auto& e : expects) {
        ASSERT_TRUE(tbl.contains(e.section)) << e.section;
        auto* section = tbl[e.section].as_table();
        ASSERT_NE(nullptr, section);
        ASSERT_TRUE(section->contains(e.moduleName)) << e.moduleName;
        auto* arr = section->at(e.moduleName).as_array();
        ASSERT_NE(nullptr, arr);
        ASSERT_GE(arr->size(), 2u);
        EXPECT_EQ(e.moduleId, arr->get(0)->value_or(int64_t{0}));
        EXPECT_EQ(1, arr->get(1)->value_or(int64_t{0}));
        EXPECT_EQ(0, e.moduleId % 10000); // IS_MAIN_DLL
    }
}
