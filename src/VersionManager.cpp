#include "VersionManager.h"
#include "AppVersion.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

core::Version VersionManager::Current()
{
    // AppVersion.hpp is the single source of truth; parse cannot fail here.
    auto v = core::Version::parse(AUK_APP_VERSION_STR);
    return v.value_or(core::Version{ AUK_VER_MAJOR, AUK_VER_MINOR, AUK_VER_PATCH });
}

std::optional<core::Version> VersionManager::ReadMarker(const std::wstring& path)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in.is_open())
        return std::nullopt;
    std::string line;
    std::getline(in, line);
    return core::Version::parse(line);
}

bool VersionManager::WriteMarker(const std::wstring& path, const core::Version& v)
{
    std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return false;
    out << v.toString() << "\n";
    return static_cast<bool>(out);
}
