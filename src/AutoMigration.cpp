#include "AutoMigration.h"
#include "Logger.h"
#include "core/Migration.hpp"

#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {
std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::string Trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Collect the ".json" stems in a directory (layout ids). Missing dir -> empty.
std::vector<std::string> LayoutIdsIn(const std::wstring& dir)
{
    std::vector<std::string> ids;
    std::error_code ec;
    if (!fs::is_directory(fs::path(dir), ec)) return ids;
    for (const auto& e : fs::directory_iterator(fs::path(dir), ec)) {
        if (ec) break;
        if (e.is_regular_file() && e.path().extension() == L".json")
            ids.push_back(e.path().stem().string());
    }
    return ids;
}
} // namespace

int AutoMigration::EnsureDirectories(const Paths& paths)
{
    int created = 0;
    std::error_code ec;
    for (const std::wstring* d : { &paths.dataRoot, &paths.userLayoutsDir }) {
        if (d->empty()) continue;
        if (!fs::exists(fs::path(*d), ec)) {
            if (fs::create_directories(fs::path(*d), ec)) {
                ++created;
                Logger::Instance().Info(L"Created directory: " + *d);
            } else {
                Logger::Instance().Error(L"Failed to create directory: " + *d);
            }
        }
    }
    return created;
}

int AutoMigration::SeedLayouts(const Paths& paths)
{
    // Nothing to copy when the bundled dir is the user dir (portable mode).
    std::error_code ec;
    if (fs::path(paths.bundledLayoutsDir) == fs::path(paths.userLayoutsDir))
        return 0;
    if (!fs::is_directory(fs::path(paths.bundledLayoutsDir), ec))
        return 0;

    auto bundled  = LayoutIdsIn(paths.bundledLayoutsDir);
    auto existing = LayoutIdsIn(paths.userLayoutsDir);
    auto toCopy   = core::Migration::planLayoutSeed(bundled, existing);

    int copied = 0;
    for (const auto& id : toCopy) {
        fs::path src = fs::path(paths.bundledLayoutsDir) / (id + ".json");
        fs::path dst = fs::path(paths.userLayoutsDir)    / (id + ".json");
        // copy_file without overwrite: preserves any user file that appears mid-run.
        if (fs::copy_file(src, dst, fs::copy_options::skip_existing, ec)) {
            ++copied;
            Logger::Instance().Info(L"Seeded layout: " + Utf8ToWide(id));
        } else if (ec) {
            Logger::Instance().Warn(L"Could not seed layout: " + Utf8ToWide(id));
            ec.clear();
        }
    }
    return copied;
}

int AutoMigration::MigrateConfigFile(const core::InstallationStatus& status,
                                     const Paths& paths)
{
    // Only rewrite when there is an existing config to migrate. A fresh install
    // has no config yet; Config will write defaults on first save.
    std::error_code ec;
    if (!fs::exists(fs::path(paths.configPath), ec))
        return 0;

    // Read existing config.ini into a key=value map (comments/sections ignored).
    core::ConfigMap existing;
    {
        std::ifstream in(fs::path(paths.configPath), std::ios::binary);
        if (!in.is_open()) return 0;
        std::string line;
        while (std::getline(in, line)) {
            std::string s = Trim(line);
            if (s.empty() || s[0] == '#' || s[0] == '[') continue;
            auto eq = s.find('=');
            if (eq == std::string::npos) continue;
            existing[Trim(s.substr(0, eq))] = Trim(s.substr(eq + 1));
        }
    }

    core::Version from = status.previous.value_or(core::Version{});
    core::ConfigMap migrated = core::Migration::migrateConfig(existing, from, status.current);
    if (migrated == existing)
        return 0; // nothing changed — leave the file untouched

    std::ofstream out(fs::path(paths.configPath), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::Instance().Error(L"Config migration: cannot write config.ini");
        return 0;
    }
    out << "# AncientUyghurKeyboard configuration (migrated)\n";
    for (const auto& [k, v] : migrated)
        out << k << "=" << v << "\n";
    Logger::Instance().Info(L"Migrated config.ini to current schema");
    return 1;
}

int AutoMigration::Run(const core::InstallationStatus& status, const Paths& paths)
{
    int actions = 0;
    actions += EnsureDirectories(paths);
    actions += SeedLayouts(paths);
    if (status.needsMigration())
        actions += MigrateConfigFile(status, paths);
    return actions;
}
