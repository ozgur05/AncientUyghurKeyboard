#include "InstallerHelper.h"
#include "VersionManager.h"
#include "AutoMigration.h"
#include "Logger.h"

#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}
std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}
} // namespace

std::wstring InstallerHelper::ExeDir()
{
    wchar_t buf[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf);
    size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? L"." : path.substr(0, slash);
}

std::wstring InstallerHelper::AppDataDir()
{
    wchar_t* raw = nullptr;
    std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw))) {
        dir = raw;
        CoTaskMemFree(raw);
    }
    dir += L"\\AncientUyghurKeyboard";
    return dir;
}

bool InstallerHelper::PortableMarkerPresent()
{
    std::error_code ec;
    return fs::exists(fs::path(ExeDir()) / L"portable.ini", ec);
}

InstallerHelper::Result InstallerHelper::Startup()
{
    Result r;
    r.portable = PortableMarkerPresent();

    const std::wstring exeDir  = ExeDir();
    const std::wstring appData = AppDataDir();

    // Resolve the writable data root via the portable-aware core rule.
    const std::string rootUtf8 = core::InstallationDetector::chooseDataRoot(
        r.portable, WideToUtf8(exeDir), WideToUtf8(appData));
    r.dataRoot   = Utf8ToWide(rootUtf8);
    r.layoutsDir = r.dataRoot + L"\\layouts";

    const std::wstring markerPath = r.dataRoot + L"\\installed_version.txt";
    const std::wstring configPath = r.dataRoot + L"\\config.ini";

    // Gather facts for detection.
    std::error_code ec;
    const bool dataDirExisted = fs::exists(fs::path(r.dataRoot), ec);
    const bool configExisted  = fs::exists(fs::path(configPath), ec);
    const auto stored         = VersionManager::ReadMarker(markerPath);
    const auto current        = VersionManager::Current();

    r.status = core::InstallationDetector::detect(stored, current,
                                                  configExisted, dataDirExisted);

    // Apply migration (creates data root + layouts, seeds layouts, migrates cfg).
    AutoMigration::Paths p;
    p.dataRoot          = r.dataRoot;
    p.userLayoutsDir    = r.layoutsDir;
    p.bundledLayoutsDir = exeDir + L"\\layouts";
    p.configPath        = configPath;
    const int actions = AutoMigration::Run(r.status, p);

    // Stamp the current version so the next launch is classified correctly.
    VersionManager::WriteMarker(markerPath, current);

    (void)actions; // count is informational; Application logs the outcome
    return r;
}
