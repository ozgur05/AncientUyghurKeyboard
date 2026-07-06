#include "Application.h"
#include "Logger.h"
#include "resource.h"

#include <shellapi.h>
#include <string>

namespace {
constexpr wchar_t kWndClass[]    = L"AncientUyghurKeyboardWnd";
constexpr UINT    kTrayCallback  = WM_APP + 1;
constexpr UINT    kTrayUID       = 1;
constexpr UINT    kReloadTimer   = 1;
constexpr UINT    kReloadMs      = 1000;

// Context-menu command IDs.
constexpr UINT    kCmdToggle     = 100;
constexpr UINT    kCmdExit       = 101;
constexpr UINT    kCmdLayoutBase = 1000; // layout items occupy [base, base+N)

HICON LoadAppIcon(HINSTANCE hInst)
{
    HICON ico = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APPICON));
    return ico ? ico : LoadIconW(nullptr, IDI_APPLICATION);
}

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

bool FileWriteTime(const std::wstring& path, FILETIME& out)
{
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
        return false;
    out = fad.ftLastWriteTime;
    return true;
}
} // namespace

int Application::Run(HINSTANCE hInstance)
{
    m_hInstance = hInstance;

    m_config.Load();
    Logger::Instance().Init(m_config.LogPath(), m_config.GetLogLevel());
    Logger::Instance().Info(L"=== AncientUyghurKeyboard starting ===");

    if (!InitInstance()) {
        Logger::Instance().Error(L"InitInstance failed");
        return 1;
    }

    // Layouts: scan the directory, activate the configured (or first) layout,
    // and wire the engine. If none load, the app still runs (keys pass through).
    InitLayouts();
    m_engine.Enable(m_config.Enabled());

    if (!m_engine.Install()) {
        MessageBoxW(nullptr, L"Failed to install keyboard hook.",
                    L"AncientUyghurKeyboard", MB_ICONERROR);
        Shutdown();
        return 2;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Shutdown();
    Logger::Instance().Info(L"=== AncientUyghurKeyboard stopped ===");
    return static_cast<int>(msg.wParam);
}

bool Application::InitInstance()
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = &Application::WndProc;
    wc.hInstance     = m_hInstance;
    wc.lpszClassName = kWndClass;
    wc.hIcon         = LoadAppIcon(m_hInstance);
    if (!RegisterClassExW(&wc))
        return false;

    m_hwnd = CreateWindowExW(0, kWndClass, L"AncientUyghurKeyboard",
                             0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                             m_hInstance, this);
    if (!m_hwnd)
        return false;

    AddTrayIcon();
    SetTimer(m_hwnd, kReloadTimer, kReloadMs, nullptr);
    return true;
}

bool Application::InitLayouts()
{
    // Re-point the engine whenever the active layout changes.
    m_manager.setOnChange([this](const core::KeyboardLayout* l) {
        m_engine.SetLayout(l);
    });

    const std::string dir       = WideToUtf8(Config::ExeDir() + L"\\layouts");
    const std::string preferred = WideToUtf8(m_config.LayoutName());

    std::string err;
    if (!m_manager.initialize(dir, preferred, &err)) {
        Logger::Instance().Error(L"Layout init failed: " + Utf8ToWide(err));
        for (const auto& e : m_manager.lastErrors())
            Logger::Instance().Error(L"layout: " + Utf8ToWide(e));
        Notify(L"Layout error", L"No usable layout; see log. Typing passes through.", true);
        return false;
    }

    for (const auto& w : m_manager.lastWarnings())
        Logger::Instance().Warn(L"layout: " + Utf8ToWide(w));

    UpdateWatchState();
    UpdateTrayTip();
    Logger::Instance().Info(L"Active layout: " + Utf8ToWide(m_manager.currentId()));
    return true;
}

void Application::SwitchLayout(const std::string& id)
{
    if (!m_manager.switchTo(id)) {
        for (const auto& e : m_manager.lastErrors())
            Logger::Instance().Error(L"layout: " + Utf8ToWide(e));
        Notify(L"Layout switch failed", Utf8ToWide(id).c_str(), true);
        return;
    }
    for (const auto& w : m_manager.lastWarnings())
        Logger::Instance().Warn(L"layout: " + Utf8ToWide(w));

    m_config.SetLayoutName(Utf8ToWide(id)); // persist choice on next Save()
    UpdateWatchState();
    UpdateTrayTip();
    Notify(L"Layout switched", Utf8ToWide(id).c_str(), false);
}

void Application::UpdateWatchState()
{
    m_watchPath.clear();
    m_watchWriteTime = FILETIME{};
    const std::string& id = m_manager.currentId();
    for (const auto& info : m_manager.available()) {
        if (info.id == id) {
            m_watchPath = Utf8ToWide(info.path);
            FileWriteTime(m_watchPath, m_watchWriteTime);
            break;
        }
    }
}

void Application::CheckLayoutReload()
{
    if (m_watchPath.empty())
        return;
    FILETIME ft{};
    if (!FileWriteTime(m_watchPath, ft))
        return; // file briefly gone (editor rewriting) — retry next tick
    if (CompareFileTime(&ft, &m_watchWriteTime) == 0)
        return;

    m_watchWriteTime = ft; // record now so a bad file doesn't retry every tick
    if (m_manager.reloadCurrent()) {
        for (const auto& w : m_manager.lastWarnings())
            Logger::Instance().Warn(L"layout: " + Utf8ToWide(w));
        Notify(L"Layout reloaded", Utf8ToWide(m_manager.currentId()).c_str(), false);
    } else {
        for (const auto& e : m_manager.lastErrors())
            Logger::Instance().Error(L"layout: " + Utf8ToWide(e));
        Notify(L"Layout reload failed", L"Kept the previous layout; see log.", true);
    }
}

void Application::Shutdown()
{
    if (m_hwnd)
        KillTimer(m_hwnd, kReloadTimer);
    m_engine.Uninstall();
    RemoveTrayIcon();
    m_config.SetEnabled(m_engine.Enabled());
    m_config.Save();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void Application::AddTrayIcon()
{
    m_nid = {};
    m_nid.cbSize           = sizeof(m_nid);
    m_nid.hWnd             = m_hwnd;
    m_nid.uID              = kTrayUID;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = kTrayCallback;
    m_nid.hIcon            = LoadAppIcon(m_hInstance);
    UpdateTrayTip();
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

void Application::RemoveTrayIcon()
{
    if (m_nid.cbSize)
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
    m_nid = {};
}

void Application::UpdateTrayTip()
{
    const wchar_t* state = m_engine.Enabled() ? L"ON" : L"OFF";
    std::wstring layout = Utf8ToWide(m_manager.currentId());
    if (layout.empty()) layout = L"(none)";
    swprintf_s(m_nid.szTip, L"Ancient Uyghur Keyboard [%s] — %s", state, layout.c_str());
    if (m_nid.cbSize)
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void Application::ToggleEnabled()
{
    m_engine.Enable(!m_engine.Enabled());
    UpdateTrayTip();
    Logger::Instance().Info(m_engine.Enabled() ? L"Enabled" : L"Disabled");
}

void Application::ShowContextMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (m_engine.Enabled() ? MF_CHECKED : MF_UNCHECKED),
                kCmdToggle, L"Enabled");

    // Layout submenu: one checkable item per available layout.
    HMENU sub = CreatePopupMenu();
    m_menuLayoutIds.clear();
    const std::string& curId = m_manager.currentId();
    UINT idx = 0;
    for (const auto& info : m_manager.available()) {
        UINT flags = MF_STRING;
        if (info.id == curId) flags |= MF_CHECKED;
        if (!info.valid)      flags |= MF_GRAYED;
        std::wstring label = Utf8ToWide(info.name);
        if (!info.valid) label += L" (invalid)";
        AppendMenuW(sub, flags, kCmdLayoutBase + idx, label.c_str());
        m_menuLayoutIds.push_back(info.id);
        ++idx;
    }
    if (idx == 0)
        AppendMenuW(sub, MF_STRING | MF_GRAYED, 0, L"(no layouts found)");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(sub), L"Layout");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCmdExit, L"Exit");

    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, nullptr);
    DestroyMenu(menu); // also destroys the submenu
}

void Application::Notify(const wchar_t* title, const wchar_t* text, bool error)
{
    if (!m_nid.cbSize) return;
    NOTIFYICONDATAW nid = m_nid;
    nid.uFlags      = NIF_INFO;
    nid.dwInfoFlags = error ? NIIF_ERROR : NIIF_INFO;
    wcscpy_s(nid.szInfoTitle, title);
    wcscpy_s(nid.szInfo, text);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Application* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Application*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self)
        return self->HandleMessage(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT Application::HandleMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case kTrayCallback:
            if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_CONTEXTMENU)
                ShowContextMenu();
            else if (LOWORD(lp) == WM_LBUTTONDBLCLK)
                ToggleEnabled();
            return 0;

        case WM_COMMAND: {
            UINT id = LOWORD(wp);
            if (id == kCmdToggle) { ToggleEnabled(); return 0; }
            if (id == kCmdExit)   { PostMessageW(hwnd, WM_CLOSE, 0, 0); return 0; }
            if (id >= kCmdLayoutBase &&
                id < kCmdLayoutBase + m_menuLayoutIds.size()) {
                SwitchLayout(m_menuLayoutIds[id - kCmdLayoutBase]);
                return 0;
            }
            break;
        }

        case WM_TIMER:
            if (wp == kReloadTimer)
                CheckLayoutReload();
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
