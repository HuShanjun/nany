#pragma once

#include <string>

enum class EAppType {
    Gate,
    Login,
    Game,
    Master,
    Admin,
    Count,
};

std::string GetAppTypeName(EAppType eAppType);
EAppType GetAppType(const std::string &strAppType);

enum class EGameAppCompID {
    NetComp,
    Count,
};