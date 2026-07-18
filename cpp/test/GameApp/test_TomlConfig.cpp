// 纯 toml++ 解析测试：锁定 CGameApp::LoadConfig / GetLogLevel / IsShowConsole /
// IsOpenLogFile / LoadServerInfo（source/GameApp/GameApp.cpp）所依赖的 toml
// key 形状（[AppSetting].LogLevel/ShowConsole/OpenLogFile、[Server.<name>]
// .type/.ServerID），不构造 CGameApp、不链接 GameApp 可执行程序。
#include <gtest/gtest.h>
#include "toml++/toml.hpp"

namespace {

constexpr uint32_t kDefaultLogLevel = 2; // Gamma::ELogLevel::eLL_Info

} // namespace

TEST(GameAppTomlConfig_Unit, AppSettingLogLevelParsed)
{
    toml::table tbl = toml::parse(R"(
        [AppSetting]
        LogLevel = 3
        ShowConsole = true
        OpenLogFile = true
    )");

    ASSERT_TRUE(tbl.contains("AppSetting"));
    uint32_t nLogLevel = tbl["AppSetting"]["LogLevel"].value<uint32_t>().value_or(kDefaultLogLevel);
    EXPECT_EQ(3u, nLogLevel);

    bool bShowConsole = tbl["AppSetting"]["ShowConsole"].value<bool>().value_or(false);
    EXPECT_TRUE(bShowConsole);

    bool bOpenLogFile = tbl["AppSetting"]["OpenLogFile"].value<bool>().value_or(false);
    EXPECT_TRUE(bOpenLogFile);
}

TEST(GameAppTomlConfig_Unit, AppSettingMissingFallsBackToDefaults)
{
    toml::table tbl = toml::parse(R"(
        [Server]
    )");

    uint32_t nLogLevel = tbl["AppSetting"]["LogLevel"].value<uint32_t>().value_or(kDefaultLogLevel);
    EXPECT_EQ(kDefaultLogLevel, nLogLevel);

    bool bShowConsole = tbl["AppSetting"]["ShowConsole"].value<bool>().value_or(false);
    EXPECT_FALSE(bShowConsole);

    bool bOpenLogFile = tbl["AppSetting"]["OpenLogFile"].value<bool>().value_or(false);
    EXPECT_FALSE(bOpenLogFile);
}

TEST(GameAppTomlConfig_Unit, ServerEntryLookupByName)
{
    toml::table tbl = toml::parse(R"(
        [AppSetting]
        LogLevel = 2

        [Server.GameServer1]
        type = "game"
        ServerID = 101
    )");

    toml::table* pServerList = tbl["Server"].as_table();
    ASSERT_NE(nullptr, pServerList);
    ASSERT_TRUE(pServerList->contains("GameServer1"));

    toml::table* pServerInfo = pServerList->at("GameServer1").as_table();
    ASSERT_NE(nullptr, pServerInfo);

    std::string strType = pServerInfo->at("type").value_or("");
    uint32_t nServerID = pServerInfo->at("ServerID").value_or(0u);

    EXPECT_EQ("game", strType);
    EXPECT_EQ(101u, nServerID);

    uint32_t nServerTypeID = nServerID / 100;
    EXPECT_EQ(1u, nServerTypeID);
}

TEST(GameAppTomlConfig_Unit, ServerEntryMissingNameNotFound)
{
    toml::table tbl = toml::parse(R"(
        [Server.GameServer1]
        type = "game"
        ServerID = 101
    )");

    toml::table* pServerList = tbl["Server"].as_table();
    ASSERT_NE(nullptr, pServerList);
    EXPECT_FALSE(pServerList->contains("UnknownServer"));
}
