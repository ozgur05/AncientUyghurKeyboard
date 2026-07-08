// DesignerWindow.h — the main layout-designer window.
//
// Owns a core::LayoutDocument and presents it: an owner-drawn keyboard canvas
// (ANSI/ISO), a layer selector, a live-typing preview, a validation panel, a
// toolbar and status bar, plus the full File/Edit/View/Layout menus wired to
// the document's edit + undo/redo operations. All model logic lives in the
// tested core; this class is Win32 glue.
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "../core/LayoutDocument.hpp"
#include "../core/KeyCaps.hpp"

class DesignerWindow {
public:
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    bool Create();
    void BuildMenu();
    void Layout();                 // reposition child controls on resize
    void PaintCanvas(HDC hdc, const RECT& area);
    unsigned HitTestCanvas(int px, int py) const;
    RECT CanvasRect() const;

    // Commands.
    void OnCommand(int id);
    void DoNew(); void DoOpen(); void DoImport(); bool DoSave(); bool DoSaveAs();
    void DoExport(); void DoEditSelectedKey(); void DoMetadata(); void DoDuplicate();
    void RefreshValidation();
    void RefreshTitle();
    void SetLayer(core::Level l);
    void SetBoard(core::BoardType b);

    // File helpers.
    bool ReadFile(const std::wstring& path, std::string& out);
    bool WriteFileBackup(const std::wstring& path, const std::string& data);
    bool ConfirmDiscardIfDirty();

    // Recent layouts (persisted to a small text file under %APPDATA%).
    void LoadRecent(); void SaveRecent(); void AddRecent(const std::wstring& path);
    void RebuildRecentMenu();
    std::wstring RecentStorePath() const;

    // Preview subclass callback.
    static LRESULT CALLBACK PreviewProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    void FeedPreviewChar(wchar_t ch);

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(HWND, UINT, WPARAM, LPARAM);

    HINSTANCE m_hInst = nullptr;
    HWND      m_hwnd  = nullptr;
    HWND      m_toolbar = nullptr;   // simple button band
    HWND      m_status  = nullptr;
    HWND      m_preview = nullptr;
    HWND      m_validation = nullptr;
    HMENU     m_menu = nullptr;
    HMENU     m_recentMenu = nullptr;
    HFONT     m_capFont = nullptr;
    HFONT     m_glyphFont = nullptr;

    core::LayoutDocument m_doc;
    core::BoardType      m_board = core::BoardType::ANSI;
    core::Level          m_layer = core::Level::Base;
    unsigned             m_selectedVk = 0;
    std::wstring         m_filePath;      // empty => untitled
    std::vector<std::wstring> m_recent;
};
