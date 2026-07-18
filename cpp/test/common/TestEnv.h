#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>

namespace GammaTest {

inline std::string GetEnvOr(const char* key, const char* fallback = "")
{
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string(fallback);
}

struct DbConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 3306;
    std::string user = "root";
    std::string password;
    std::string database = "nany_test";
};

inline DbConfig LoadDbConfigFromEnv()
{
    DbConfig c;
    c.host = GetEnvOr("NANY_TEST_DB_HOST", "127.0.0.1");
    const std::string portStr = GetEnvOr("NANY_TEST_DB_PORT", "3306");
    c.port = static_cast<uint16_t>(std::stoi(portStr.empty() ? "3306" : portStr));
    c.user = GetEnvOr("NANY_TEST_DB_USER", "root");
    c.password = GetEnvOr("NANY_TEST_DB_PASSWORD", "");
    c.database = GetEnvOr("NANY_TEST_DB_DATABASE", "nany_test");
    return c;
}

} // namespace GammaTest
