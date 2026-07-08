// Application.h — top-level app: tray icon, menu, message loop, lifecycle.
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "Config.h"
#include "KeyboardEngine.h"
#include "InstallerHelper.h"
#include "win/ScopedResources.h"
#include "core/KeyboardLayoutManager.hpp"

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
    void ReportInstallStatus();   // log + toast first-install / upgrade

    // Layout management.
    bool InitLayouts();                       // scan + activate + wire engine
    void SwitchLayout(const std::string& id); // runtime switch
    void UpdateWatchState();                   // track current file for hot reload
    void CheckLayoutReload();                  // poll current file; reload on change

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

    HINSTANCE       m_hInstance     = nullptr;
    HWND            m_hwnd          = nullptr;
    ScopedHandle    m_instanceMutex;           // single-instance / installer AppMutex (RAII)
    NOTIFYICONDATAW m_nid           = {};

    Config                        m_config;
    core::KeyboardLayoutManager   m_manager;   // owns layouts + active pointer
    KeyboardEngine                m_engine;
    InstallerHelper::Result       m_install;   // resolved data root + launch kind

    // Menu id -> layout id, rebuilt each time the context menu is shown.
    std::vector<std::string> m_menuLayoutIds;

    // Hot-reload watch of the active layout file.
    std::wstring m_watchPath;
    FILETIME     m_watchWriteTime = {};
};
