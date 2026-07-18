#pragma once
#include <filesystem>
#include <random>
#include <string>

namespace GammaTest {

class TempDir {
public:
    TempDir()
    {
        auto base = std::filesystem::temp_directory_path() / "nany_gtest";
        std::filesystem::create_directories(base);
        path_ = base / ("t_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(path_);
    }
    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace GammaTest
