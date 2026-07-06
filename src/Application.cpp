#include "Application.h"
#include "Logger.h"
#include "resource.h"
#include "core/LayoutLoader.hpp"

#include <shellapi.h>
#include <fstream>
#include <sstream>

namespace {
constexpr wchar_t kWndClass[] = L"AncientUyghurKeyboardWnd";
constexpr UINT    kTrayCallback = WM_APP + 1;
constexpr UINT    kTrayUID      = 1;
constexpr UINT    kReloadTimer  = 1;      // hot-reload polling timer
constexpr UINT    kReloadMs     = 1000;   // poll interval

// Context-menu command IDs.
constexpr UINT    kCmdToggle = 100;
constexpr UINT    kCmdExit   = 101;

// Load the bundled app icon if present, otherwise the system default.
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

// Read the last-write time of a file. Returns false if it can't be queried.
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

    // --- Config + logging first so everything else can log. ---
    m_config.Load();
    Logger::Instance().Init(m_config.LogPath(), m_config.GetLogLevel());
    Logger::Instance().Info(L"=== AncientUyghurKeyboard starting ===");

    // --- Window + tray first, so we can show load notifications. ---
    if (!InitInstance()) {
        Logger::Instance().Error(L"InitInstance failed");
        return 1;
    }

    // --- Layout (JSON, hot-reloadable). If it fails, the app still runs with
    //     mapping disabled (keys pass through) until a valid file appears. ---
    m_layoutPath = m_config.LayoutPath();
    LoadLayout(/*initial*/ true);

    m_engine.Enable(m_config.Enabled());
    if (!m_engine.Install()) {
        MessageBoxW(nullptr, L"Failed to install keyboard hook.",
                    L"AncientUyghurKeyboard", MB_ICONERROR);
        Shutdown();
        return 2;
    }

    // --- Message loop ---
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

    // Message-only window: no UI, just receives tray/menu messages.
    m_hwnd = CreateWindowExW(0, kWndClass, L"AncientUyghurKeyboard",
                             0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                             m_hInstance, this);
    if (!m_hwnd)
        return false;

    AddTrayIcon();
    SetTimer(m_hwnd, kReloadTimer, kReloadMs, nullptr); // hot-reload polling
    return true;
}

bool Application::LoadLayout(bool initial)
{
    // Record the current write time up front so a parse failure doesn't cause
    // the reloader to retry the same broken file every tick.
    FileWriteTime(m_layoutPath, m_layoutWriteTime);

    std::ifstream in(m_layoutPath.c_str(), std::ios::binary); // MSVC: wchar_t* overload
    if (!in.is_open()) {
        Logger::Instance().Error(L"Cannot open layout file: " + m_layoutPath);
        if (initial) Notify(L"Layout error", L"Could not open the layout file.", true);
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    core::LoadResult result = core::LayoutLoader::loadFromString(ss.str());

    for (const auto& w : result.warnings)
        Logger::Instance().Warn(L"layout: " + Utf8ToWide(w));

    if (!result.ok()) {
        for (const auto& e : result.errors)
            Logger::Instance().Error(L"layout: " + Utf8ToWide(e));
        Notify(initial ? L"Layout error" : L"Layout reload failed",
               initial ? L"Layout failed to load; see log."
                       : L"Kept the previous layout; see log.", true);
        return false;
    }

    m_layout = std::move(*result.layout);
    m_engine.SetLayout(&m_layout);
    Logger::Instance().Info(L"Layout loaded: " + Utf8ToWide(m_layout.meta().name));
    return true;
}

void Application::CheckLayoutReload()
{
    FILETIME ft{};
    if (!FileWriteTime(m_layoutPath, ft))
        return; // file temporarily gone (e.g. editor rewriting) — try again later
    if (CompareFileTime(&ft, &m_layoutWriteTime) == 0)
        return; // unchanged

    if (LoadLayout(/*initial*/ false))
        Notify(L"Layout reloaded", Utf8ToWide(m_layout.meta().name).c_str(), false);
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
    swprintf_s(m_nid.szTip, L"Ancient Uyghur Keyboard [%s]", state);
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
    UINT check = m_engine.Enabled() ? MF_CHECKED : MF_UNCHECKED;
    AppendMenuW(menu, MF_STRING | check, kCmdToggle, L"Enabled");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCmdExit, L"Exit");

    // Required so the menu dismisses correctly for a tray window.
    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwnd, nullptr);
    DestroyMenu(menu);
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

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case kCmdToggle: ToggleEnabled();          return 0;
                case kCmdExit:   PostMessageW(hwnd, WM_CLOSE, 0, 0); return 0;
            }
            break;

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
