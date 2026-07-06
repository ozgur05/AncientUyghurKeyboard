// Application.h — top-level app: tray icon, menu, message loop, lifecycle.
#pragma once

#include <windows.h>
#include <string>
#include "Config.h"
#include "KeyboardEngine.h"
#include "core/KeyboardLayout.hpp"

class Application {
public:
    Application()  = default;
    ~Application() = default;

    // Full run: init subsystems, pump messages, clean up. Returns exit code.
    int Run(HINSTANCE hInstance);

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

private:
    bool InitInstance();
    void Shutdown();

    // Layout loading + hot reload.
    bool LoadLayout(bool initial);
    void CheckLayoutReload();

    // Tray helpers.
    void AddTrayIcon();
    void RemoveTrayIcon();
    void UpdateTrayTip();
    void ShowContextMenu();
    void Notify(const wchar_t* title, const wchar_t* text, bool error);

    void ToggleEnabled();

    // Hidden window that receives tray + menu + timer messages.
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT HandleMessage(HWND, UINT, WPARAM, LPARAM);

    HINSTANCE       m_hInstance = nullptr;
    HWND            m_hwnd      = nullptr;
    NOTIFYICONDATAW m_nid       = {};

    Config               m_config;
    core::KeyboardLayout m_layout;      // owned; engine holds a pointer to it
    KeyboardEngine       m_engine;

    std::wstring m_layoutPath;          // watched file
    FILETIME     m_layoutWriteTime = {}; // last-seen modification time
};
