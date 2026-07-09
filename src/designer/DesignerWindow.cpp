#include "DesignerWindow.h"
#include "KeyEditDialog.h"
#include "UnicodePicker.h"
#include "Modal.h"
#include "../core/LayoutSerializer.hpp"
#include "../core/Composer.hpp"
#include "../core/VirtualKeys.hpp"
#include "../core/Unicode.hpp"

#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwchar>

namespace fs = std::filesystem;
using namespace core;

namespace {

// Menu / control command ids.
enum {
    IDM_NEW = 100, IDM_OPEN, IDM_IMPORT, IDM_SAVE, IDM_SAVEAS, IDM_EXPORT, IDM_EXIT,
    IDM_UNDO, IDM_REDO, IDM_COPYKEY, IDM_PASTEKEY, IDM_EDITKEY,
    IDM_ANSI, IDM_ISO,
    IDM_LAYER_BASE, IDM_LAYER_SHIFT, IDM_LAYER_ALTGR, IDM_LAYER_SAG,
    IDM_META, IDM_DUPLICATE, IDM_VALIDATE, IDM_ABOUT,
    IDC_PREVIEW = 700, IDC_VALIDATION = 701, IDC_STATUS = 702,
    IDM_RECENT_BASE = 900   // .. IDM_RECENT_BASE + N
};

constexpr int kToolbarH = 40;
constexpr int kRightW   = 300;

std::wstring toW(const std::string& s)
{
    std::u32string u32 = unicode::utf8ToUtf32(s);
    std::u16string u16 = unicode::utf32ToUtf16(u32);
    return std::wstring(reinterpret_cast<const wchar_t*>(u16.c_str()), u16.size());
}
std::string toUtf8(const std::wstring& w)
{
    std::u16string u16(w.begin(), w.end());
    return unicode::utf32ToUtf8(unicode::utf16ToUtf32(u16));
}
std::wstring glyphOf(const std::u32string& s)
{
    std::u16string u16 = unicode::utf32ToUtf16(s);
    return std::wstring(reinterpret_cast<const wchar_t*>(u16.c_str()), u16.size());
}

// Preview buffer lives with the window instance (see DesignerWindow).
} // namespace

// A tiny per-window store for the live-preview text.
static std::u32string g_previewText;
static Composer       g_previewComposer;

// ---------------------------------------------------------------------------

int DesignerWindow::Run(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    if (!Create())
        return 1;

    LoadRecent();
    RebuildRecentMenu();
    DoNew();
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    // Accelerators for the common shortcuts.
    ACCEL accels[] = {
        { FVIRTKEY | FCONTROL, 'N', IDM_NEW },
        { FVIRTKEY | FCONTROL, 'O', IDM_OPEN },
        { FVIRTKEY | FCONTROL, 'S', IDM_SAVE },
        { FVIRTKEY | FCONTROL, 'Z', IDM_UNDO },
        { FVIRTKEY | FCONTROL, 'Y', IDM_REDO },
        { FVIRTKEY | FCONTROL, 'C', IDM_COPYKEY },
        { FVIRTKEY | FCONTROL, 'V', IDM_PASTEKEY },
    };
    HACCEL hAccel = CreateAcceleratorTableW(accels, ARRAYSIZE(accels));

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!TranslateAcceleratorW(m_hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    if (hAccel) DestroyAcceleratorTable(hAccel);
    return static_cast<int>(msg.wParam);
}

bool DesignerWindow::Create()
{
    const wchar_t* kClass = L"AncientUyghurDesignerWnd";
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc   = &DesignerWindow::WndProc;
    wc.hInstance     = m_hInst;
    wc.lpszClassName = kClass;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
    if (!RegisterClassExW(&wc))
        return false;

    m_hwnd = CreateWindowExW(0, kClass, L"Ancient Uyghur Layout Designer",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 640,
        nullptr, nullptr, m_hInst, this);
    if (!m_hwnd)
        return false;

    BuildMenu();

    // Toolbar band: plain buttons reusing menu command ids.
    struct TB { const wchar_t* label; int id; } tbs[] = {
        { L"New", IDM_NEW }, { L"Open", IDM_OPEN }, { L"Save", IDM_SAVE },
        { L"Undo", IDM_UNDO }, { L"Redo", IDM_REDO },
        { L"Edit Key", IDM_EDITKEY }, { L"Validate", IDM_VALIDATE },
    };
    int x = 6;
    for (const auto& t : tbs) {
        CreateWindowExW(0, L"BUTTON", t.label, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            x, 6, 78, 28, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(t.id)),
            m_hInst, nullptr);
        x += 82;
    }

    m_status = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUS)), m_hInst, nullptr);

    // Right column: preview (top) + validation list (bottom).
    m_preview = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
        0, 0, 10, 10, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PREVIEW)), m_hInst, nullptr);
    SetWindowSubclass(m_preview, &DesignerWindow::PreviewProc, 1,
                      reinterpret_cast<DWORD_PTR>(this));

    m_validation = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
        0, 0, 10, 10, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_VALIDATION)), m_hInst, nullptr);

    m_capFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_glyphFont = CreateFontW(30, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    Layout();
    return true;
}

void DesignerWindow::BuildMenu()
{
    m_menu = CreateMenu();

    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, IDM_NEW,    L"&New\tCtrl+N");
    AppendMenuW(file, MF_STRING, IDM_OPEN,   L"&Open…\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_IMPORT, L"&Import…");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, IDM_SAVE,   L"&Save\tCtrl+S");
    AppendMenuW(file, MF_STRING, IDM_SAVEAS, L"Save &As…");
    AppendMenuW(file, MF_STRING, IDM_EXPORT, L"&Export…");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    m_recentMenu = CreatePopupMenu();
    AppendMenuW(file, MF_POPUP, reinterpret_cast<UINT_PTR>(m_recentMenu), L"&Recent");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, IDM_EXIT,   L"E&xit");

    HMENU edit = CreatePopupMenu();
    AppendMenuW(edit, MF_STRING, IDM_UNDO,     L"&Undo\tCtrl+Z");
    AppendMenuW(edit, MF_STRING, IDM_REDO,     L"&Redo\tCtrl+Y");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, IDM_COPYKEY,  L"&Copy Key\tCtrl+C");
    AppendMenuW(edit, MF_STRING, IDM_PASTEKEY, L"&Paste Key\tCtrl+V");
    AppendMenuW(edit, MF_STRING, IDM_EDITKEY,  L"&Edit Key…");

    HMENU view = CreatePopupMenu();
    AppendMenuW(view, MF_STRING, IDM_ANSI, L"&ANSI keyboard");
    AppendMenuW(view, MF_STRING, IDM_ISO,  L"&ISO keyboard");
    AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(view, MF_STRING, IDM_LAYER_BASE,  L"Layer: &Base");
    AppendMenuW(view, MF_STRING, IDM_LAYER_SHIFT, L"Layer: &Shift");
    AppendMenuW(view, MF_STRING, IDM_LAYER_ALTGR, L"Layer: &AltGr");
    AppendMenuW(view, MF_STRING, IDM_LAYER_SAG,   L"Layer: Shift+A&ltGr");

    HMENU layout = CreatePopupMenu();
    AppendMenuW(layout, MF_STRING, IDM_META,      L"&Metadata…");
    AppendMenuW(layout, MF_STRING, IDM_DUPLICATE, L"&Duplicate…");
    AppendMenuW(layout, MF_STRING, IDM_VALIDATE,  L"&Validate");

    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, IDM_ABOUT, L"&About");

    AppendMenuW(m_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file),   L"&File");
    AppendMenuW(m_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(edit),   L"&Edit");
    AppendMenuW(m_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(view),   L"&View");
    AppendMenuW(m_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(layout), L"&Layout");
    AppendMenuW(m_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(help),   L"&Help");
    SetMenu(m_hwnd, m_menu);
}

RECT DesignerWindow::CanvasRect() const
{
    RECT rc; GetClientRect(m_hwnd, &rc);
    RECT sb = {}; if (m_status) GetWindowRect(m_status, &sb);
    int statusH = sb.bottom - sb.top;
    RECT c = rc;
    c.top    += kToolbarH;
    c.bottom -= statusH;
    c.right  -= kRightW;
    return c;
}

void DesignerWindow::Layout()
{
    if (m_status) SendMessageW(m_status, WM_SIZE, 0, 0);
    RECT rc; GetClientRect(m_hwnd, &rc);
    RECT sb = {}; if (m_status) GetWindowRect(m_status, &sb);
    int statusH = sb.bottom - sb.top;

    int rightX = rc.right - kRightW + 8;
    int top = kToolbarH + 4;
    int bottom = rc.bottom - statusH - 4;
    int colW = kRightW - 16;
    int previewH = (bottom - top) / 2;

    if (m_preview)
        MoveWindow(m_preview, rightX, top, colW, previewH - 8, TRUE);
    if (m_validation)
        MoveWindow(m_validation, rightX, top + previewH, colW, bottom - (top + previewH), TRUE);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void DesignerWindow::PaintCanvas(HDC hdc, const RECT& area)
{
    const auto& caps = KeyCaps::caps(m_board);
    double unitsW = KeyCaps::width(m_board);
    double unitsH = KeyCaps::height(m_board);
    int availW = area.right - area.left - 20;
    int availH = area.bottom - area.top - 20;
    double unit = std::min(availW / unitsW, availH / unitsH);
    if (unit < 10) unit = 10;
    int ox = area.left + 10;
    int oy = area.top + 10;
    const int gap = 3;

    HBRUSH keyBrush  = CreateSolidBrush(RGB(245, 245, 248));
    HBRUSH selBrush  = CreateSolidBrush(RGB(180, 210, 255));
    HBRUSH decoBrush = CreateSolidBrush(RGB(225, 225, 228));
    HPEN   pen       = CreatePen(PS_SOLID, 1, RGB(150, 150, 155));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    SetBkMode(hdc, TRANSPARENT);

    for (const auto& c : caps) {
        RECT r;
        r.left   = ox + static_cast<int>(c.x * unit) + gap;
        r.top    = oy + static_cast<int>(c.y * unit) + gap;
        r.right  = ox + static_cast<int>((c.x + c.w) * unit) - gap;
        r.bottom = oy + static_cast<int>((c.y + c.h) * unit) - gap;

        HBRUSH b = !c.editable ? decoBrush : (c.vk == m_selectedVk ? selBrush : keyBrush);
        FillRect(hdc, &r, b);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);

        // Legend (top-left, small).
        SelectObject(hdc, m_capFont);
        SetTextColor(hdc, RGB(90, 90, 95));
        RECT lr = { r.left + 4, r.top + 2, r.right - 2, r.top + 18 };
        std::wstring label(c.label.begin(), c.label.end());
        DrawTextW(hdc, label.c_str(), -1, &lr, DT_LEFT | DT_SINGLELINE);

        // Mapped glyph for the current layer (center, large).
        if (c.editable && c.vk) {
            if (const KeyDef* def = m_doc.layout().key(c.vk)) {
                const KeyAction& a = def->levels[static_cast<size_t>(m_layer)];
                std::wstring g;
                if (a.kind == ActionKind::Emit)      g = glyphOf(a.output);
                else if (a.kind == ActionKind::DeadKey) g = L"◌ dead";
                else if (a.kind == ActionKind::Compose) g = L"⎄ comp";
                if (!g.empty()) {
                    SelectObject(hdc, m_glyphFont);
                    SetTextColor(hdc, RGB(20, 20, 30));
                    RECT gr = { r.left, r.top + 14, r.right, r.bottom - 2 };
                    DrawTextW(hdc, g.c_str(), -1, &gr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            }
        }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen); DeleteObject(keyBrush); DeleteObject(selBrush); DeleteObject(decoBrush);
}

unsigned DesignerWindow::HitTestCanvas(int px, int py) const
{
    RECT area = CanvasRect();
    const auto& caps = KeyCaps::caps(m_board);
    double unitsW = KeyCaps::width(m_board);
    double unitsH = KeyCaps::height(m_board);
    int availW = area.right - area.left - 20;
    int availH = area.bottom - area.top - 20;
    double unit = std::min(availW / unitsW, availH / unitsH);
    if (unit < 10) unit = 10;
    int ox = area.left + 10, oy = area.top + 10;

    for (const auto& c : caps) {
        if (!c.editable || !c.vk) continue;
        int l = ox + static_cast<int>(c.x * unit);
        int t = oy + static_cast<int>(c.y * unit);
        int rr = ox + static_cast<int>((c.x + c.w) * unit);
        int bb = oy + static_cast<int>((c.y + c.h) * unit);
        if (px >= l && px < rr && py >= t && py < bb) return c.vk;
    }
    return 0;
}

// ---- Commands ---------------------------------------------------------------

void DesignerWindow::OnCommand(int id)
{
    if (id >= IDM_RECENT_BASE && id < IDM_RECENT_BASE + static_cast<int>(m_recent.size())) {
        if (!ConfirmDiscardIfDirty()) return;
        std::wstring path = m_recent[id - IDM_RECENT_BASE];
        std::string data;
        std::string err;
        if (ReadFile(path, data) && m_doc.loadFromJson(data, &err)) {
            m_filePath = path; AddRecent(path); g_previewComposer.setLayout(&m_doc.layout());
            RefreshTitle(); RefreshValidation(); InvalidateRect(m_hwnd, nullptr, TRUE);
        } else {
            MessageBoxW(m_hwnd, toW(err.empty() ? "Failed to open." : err).c_str(),
                        L"Open", MB_ICONERROR);
        }
        return;
    }
    switch (id) {
        case IDM_NEW:       if (ConfirmDiscardIfDirty()) DoNew(); break;
        case IDM_OPEN:      DoOpen(); break;
        case IDM_IMPORT:    DoImport(); break;
        case IDM_SAVE:      DoSave(); break;
        case IDM_SAVEAS:    DoSaveAs(); break;
        case IDM_EXPORT:    DoExport(); break;
        case IDM_EXIT:      if (ConfirmDiscardIfDirty()) DestroyWindow(m_hwnd); break;
        case IDM_UNDO:      if (m_doc.undo()) { g_previewComposer.setLayout(&m_doc.layout()); RefreshValidation(); RefreshTitle(); InvalidateRect(m_hwnd,nullptr,TRUE);} break;
        case IDM_REDO:      if (m_doc.redo()) { g_previewComposer.setLayout(&m_doc.layout()); RefreshValidation(); RefreshTitle(); InvalidateRect(m_hwnd,nullptr,TRUE);} break;
        case IDM_COPYKEY:   if (m_selectedVk) m_doc.copyKey(m_selectedVk); break;
        case IDM_PASTEKEY:  if (m_selectedVk && m_doc.pasteKey(m_selectedVk)) { g_previewComposer.setLayout(&m_doc.layout()); RefreshValidation(); RefreshTitle(); InvalidateRect(m_hwnd,nullptr,TRUE);} break;
        case IDM_EDITKEY:   DoEditSelectedKey(); break;
        case IDM_ANSI:      SetBoard(BoardType::ANSI); break;
        case IDM_ISO:       SetBoard(BoardType::ISO); break;
        case IDM_LAYER_BASE:  SetLayer(Level::Base); break;
        case IDM_LAYER_SHIFT: SetLayer(Level::Shift); break;
        case IDM_LAYER_ALTGR: SetLayer(Level::AltGr); break;
        case IDM_LAYER_SAG:   SetLayer(Level::ShiftAltGr); break;
        case IDM_META:      DoMetadata(); break;
        case IDM_DUPLICATE: DoDuplicate(); break;
        case IDM_VALIDATE:  RefreshValidation(); break;
        case IDM_ABOUT:
            MessageBoxW(m_hwnd,
                L"Ancient Uyghur Layout Designer\n\nVisual editor for keyboard layout JSON files.",
                L"About", MB_ICONINFORMATION);
            break;
    }
}

void DesignerWindow::SetLayer(Level l) { m_layer = l; InvalidateRect(m_hwnd, nullptr, TRUE); RefreshTitle(); }
void DesignerWindow::SetBoard(BoardType b) { m_board = b; m_selectedVk = 0; InvalidateRect(m_hwnd, nullptr, TRUE); }

void DesignerWindow::DoNew()
{
    m_doc.newLayout("new_layout", "New Layout");
    m_filePath.clear();
    m_selectedVk = 0;
    g_previewComposer.setLayout(&m_doc.layout());
    g_previewText.clear();
    if (m_preview) SetWindowTextW(m_preview, L"");
    RefreshTitle(); RefreshValidation(); InvalidateRect(m_hwnd, nullptr, TRUE);
}

void DesignerWindow::DoEditSelectedKey()
{
    if (!m_selectedVk) {
        MessageBoxW(m_hwnd, L"Select a key on the keyboard first.", L"Edit Key", MB_ICONINFORMATION);
        return;
    }
    KeyDef def;
    if (const KeyDef* d = m_doc.layout().key(m_selectedVk)) def = *d;
    if (EditKey(m_hwnd, m_hInst, m_selectedVk, def)) {
        // Apply each level through the document so undo captures the whole change.
        for (int i = 0; i < static_cast<int>(Level::Count); ++i)
            m_doc.setKeyAction(m_selectedVk, static_cast<Level>(i), def.levels[i]);
        g_previewComposer.setLayout(&m_doc.layout());
        RefreshValidation(); RefreshTitle();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

// ---- File helpers -----------------------------------------------------------

bool DesignerWindow::ReadFile(const std::wstring& path, std::string& out)
{
    std::ifstream in(fs::path(path), std::ios::binary);
    if (!in.is_open()) return false;
    std::ostringstream ss; ss << in.rdbuf(); out = ss.str();
    return true;
}

bool DesignerWindow::WriteFileBackup(const std::wstring& path, const std::string& data)
{
    std::error_code ec;
    if (fs::exists(fs::path(path), ec))
        fs::copy_file(fs::path(path), fs::path(path + L".bak"),
                      fs::copy_options::overwrite_existing, ec); // best-effort backup
    std::ofstream out(fs::path(path), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(out);
}

bool DesignerWindow::ConfirmDiscardIfDirty()
{
    if (!m_doc.dirty()) return true;
    int r = MessageBoxW(m_hwnd, L"Discard unsaved changes?", L"Layout Designer",
                        MB_YESNO | MB_ICONWARNING);
    return r == IDYES;
}

// The keyboard app's user layouts directory. Saving here means the running
// keyboard hot-reloads the layout automatically after Save.
static std::wstring UserLayoutsDir()
{
    wchar_t* raw = nullptr; std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw))) {
        dir = raw; CoTaskMemFree(raw);
    }
    dir += L"\\AncientUyghurKeyboard\\layouts";
    return dir;
}

static bool BrowseFile(HWND owner, bool save, std::wstring& path)
{
    wchar_t buf[MAX_PATH]; buf[0] = 0;
    if (!path.empty()) wcsncpy_s(buf, path.c_str(), _TRUNCATE);
    std::wstring initial = UserLayoutsDir();
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Layout JSON (*.json)\0*.json\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.lpstrInitialDir = path.empty() ? initial.c_str() : nullptr;
    ofn.Flags = save ? (OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST)
                     : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
    BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return false;
    path = buf;
    return true;
}

void DesignerWindow::DoOpen()
{
    if (!ConfirmDiscardIfDirty()) return;
    std::wstring path = m_filePath;
    if (!BrowseFile(m_hwnd, false, path)) return;
    std::string data, err;
    if (ReadFile(path, data) && m_doc.loadFromJson(data, &err)) {
        m_filePath = path; AddRecent(path); g_previewComposer.setLayout(&m_doc.layout());
        RefreshTitle(); RefreshValidation(); InvalidateRect(m_hwnd, nullptr, TRUE);
    } else {
        MessageBoxW(m_hwnd, toW(err.empty() ? "Failed to open file." : err).c_str(),
                    L"Open", MB_ICONERROR);
    }
}

void DesignerWindow::DoImport() { DoOpen(); } // import == open a layout to edit

bool DesignerWindow::DoSave()
{
    if (m_filePath.empty()) return DoSaveAs();
    auto rep = m_doc.validate();
    if (!rep.ok()) {
        RefreshValidation();
        if (MessageBoxW(m_hwnd, L"The layout has validation errors and may not load correctly.\n"
                        L"Save anyway?", L"Save", MB_YESNO | MB_ICONWARNING) != IDYES)
            return false;
    }
    if (!WriteFileBackup(m_filePath, m_doc.toJson())) {
        MessageBoxW(m_hwnd, L"Failed to write file.", L"Save", MB_ICONERROR);
        return false;
    }
    m_doc.markSaved(); AddRecent(m_filePath); RefreshTitle();
    return true;
}

bool DesignerWindow::DoSaveAs()
{
    std::wstring path = m_filePath;
    if (!BrowseFile(m_hwnd, true, path)) return false;
    m_filePath = path;
    return DoSave();
}

void DesignerWindow::DoExport()
{
    std::wstring path;
    if (!BrowseFile(m_hwnd, true, path)) return;
    if (!WriteFileBackup(path, m_doc.toJson()))
        MessageBoxW(m_hwnd, L"Failed to export.", L"Export", MB_ICONERROR);
}

// ---- Metadata + duplicate (simple modal dialogs) ----------------------------

namespace {
struct MetaState {
    bool done = false, accepted = false;
    HWND id{}, name{}, lang{}, author{}, ver{}, desc{};
    LayoutMeta result;
};
LRESULT CALLBACK MetaProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* st = reinterpret_cast<MetaState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_COMMAND && st) {
        int id = LOWORD(wp);
        if (id == IDOK) {
            wchar_t b[512];
            auto get = [&](HWND h){ GetWindowTextW(h, b, 511); return toUtf8(b); };
            st->result.id = get(st->id);
            st->result.name = get(st->name);
            st->result.language = get(st->lang);
            st->result.author = get(st->author);
            st->result.description = get(st->desc);
            GetWindowTextW(st->ver, b, 511);
            try { st->result.version = std::stoi(b); } catch (...) { st->result.version = 1; }
            st->accepted = true; st->done = true; return 0;
        }
        if (id == IDCANCEL) { st->done = true; return 0; }
    }
    if (msg == WM_CLOSE && st) { st->done = true; return 0; }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
} // namespace

void DesignerWindow::DoMetadata()
{
    static bool reg = false; const wchar_t* cls = L"AUKMetaDialog";
    if (!reg) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = MetaProc; wc.hInstance = m_hInst; wc.lpszClassName = cls;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        RegisterClassExW(&wc); reg = true;
    }
    MetaState st;
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, cls, L"Layout Metadata",
        WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 460, 320,
        m_hwnd, nullptr, m_hInst, nullptr);
    if (!dlg) return;
    SetWindowLongPtrW(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&st));
    auto mk = [&](const wchar_t* cl, const wchar_t* t, DWORD s, int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cl, t, WS_CHILD | WS_VISIBLE | s, x, y, w, h, dlg,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), m_hInst, nullptr);
    };
    const LayoutMeta& m = m_doc.layout().meta();
    struct Row { const wchar_t* label; HWND* out; std::wstring val; } rows[] = {
        { L"Identifier", &st.id,     toW(m.id) },
        { L"Name",       &st.name,   toW(m.name) },
        { L"Language",   &st.lang,   toW(m.language) },
        { L"Author",     &st.author, toW(m.author) },
        { L"Version",    &st.ver,    std::to_wstring(m.version) },
    };
    int y = 14;
    for (auto& r : rows) {
        mk(L"STATIC", r.label, 0, 14, y + 3, 90, 20, -1);
        *r.out = mk(L"EDIT", r.val.c_str(), WS_BORDER | ES_AUTOHSCROLL, 110, y, 320, 24, -1);
        y += 34;
    }
    mk(L"STATIC", L"Description", 0, 14, y + 3, 90, 20, -1);
    st.desc = mk(L"EDIT", toW(m.description).c_str(), WS_BORDER | ES_AUTOHSCROLL, 110, y, 320, 24, -1);
    y += 40;
    mk(L"BUTTON", L"OK",     BS_DEFPUSHBUTTON, 250, y, 84, 28, IDOK);
    mk(L"BUTTON", L"Cancel", 0,                340, y, 84, 28, IDCANCEL);

    RunModal(m_hwnd, dlg, st.done);
    if (st.accepted) { m_doc.setMeta(st.result); RefreshTitle(); RefreshValidation(); }
}

void DesignerWindow::DoDuplicate()
{
    // Duplicate = keep current mappings but start a new unsaved file with a new id.
    LayoutMeta m = m_doc.layout().meta();
    m.id   += "_copy";
    m.name += " (copy)";
    m_doc.setMeta(m);
    m_filePath.clear();
    RefreshTitle();
    MessageBoxW(m_hwnd, L"Created an unsaved duplicate. Use Save As to write it to a new file.",
                L"Duplicate", MB_ICONINFORMATION);
}

// ---- Validation / title -----------------------------------------------------

void DesignerWindow::RefreshValidation()
{
    SendMessageW(m_validation, LB_RESETCONTENT, 0, 0);
    auto rep = m_doc.validate();
    for (const auto& e : rep.errors)
        SendMessageW(m_validation, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>((L"ERROR: " + toW(e)).c_str()));
    for (const auto& w : rep.warnings)
        SendMessageW(m_validation, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>((L"warn: " + toW(w)).c_str()));
    if (rep.errors.empty() && rep.warnings.empty())
        SendMessageW(m_validation, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Layout is valid."));

    wchar_t status[128];
    swprintf_s(status, L"Keys: %zu   Errors: %zu   Warnings: %zu",
               m_doc.layout().keys().size(), rep.errors.size(), rep.warnings.size());
    SendMessageW(m_status, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(status));
}

void DesignerWindow::RefreshTitle()
{
    const wchar_t* layerName =
        m_layer == Level::Base ? L"Base" : m_layer == Level::Shift ? L"Shift" :
        m_layer == Level::AltGr ? L"AltGr" : L"Shift+AltGr";
    std::wstring name = m_filePath.empty() ? L"(untitled)" : fs::path(m_filePath).filename().wstring();
    std::wstring title = L"Ancient Uyghur Layout Designer — " + name +
        (m_doc.dirty() ? L" *" : L"") + L"   [layer: " + layerName + L"]";
    SetWindowTextW(m_hwnd, title.c_str());
}

// ---- Recent -----------------------------------------------------------------

std::wstring DesignerWindow::RecentStorePath() const
{
    wchar_t* raw = nullptr; std::wstring dir;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &raw))) {
        dir = raw; CoTaskMemFree(raw);
    }
    dir += L"\\AncientUyghurKeyboard";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\designer_recent.txt";
}

void DesignerWindow::LoadRecent()
{
    m_recent.clear();
    std::ifstream in(fs::path(RecentStorePath()), std::ios::binary);
    std::string line;
    while (std::getline(in, line) && m_recent.size() < 10) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) m_recent.push_back(toW(line));
    }
}

void DesignerWindow::SaveRecent()
{
    std::ofstream out(fs::path(RecentStorePath()), std::ios::binary | std::ios::trunc);
    for (const auto& p : m_recent) out << toUtf8(p) << "\n";
}

void DesignerWindow::AddRecent(const std::wstring& path)
{
    m_recent.erase(std::remove(m_recent.begin(), m_recent.end(), path), m_recent.end());
    m_recent.insert(m_recent.begin(), path);
    if (m_recent.size() > 10) m_recent.resize(10);
    SaveRecent();
    RebuildRecentMenu();
}

void DesignerWindow::RebuildRecentMenu()
{
    if (!m_recentMenu) return;
    while (GetMenuItemCount(m_recentMenu) > 0) DeleteMenu(m_recentMenu, 0, MF_BYPOSITION);
    if (m_recent.empty()) {
        AppendMenuW(m_recentMenu, MF_STRING | MF_GRAYED, 0, L"(none)");
        return;
    }
    for (size_t i = 0; i < m_recent.size(); ++i) {
        std::wstring label = std::to_wstring(i + 1) + L"  " + fs::path(m_recent[i]).filename().wstring();
        AppendMenuW(m_recentMenu, MF_STRING, IDM_RECENT_BASE + i, label.c_str());
    }
}

// ---- Live preview -----------------------------------------------------------

void DesignerWindow::FeedPreviewChar(wchar_t ch)
{
    if (ch == L'\b') {            // Backspace edits the preview buffer.
        if (!g_previewText.empty()) g_previewText.pop_back();
    } else {
        SHORT sc = VkKeyScanW(ch);
        if (sc != -1) {
            KeyInput in;
            in.vk    = static_cast<unsigned>(sc & 0xFF);
            in.shift = (sc & 0x100) != 0;
            EmitOp op = g_previewComposer.process(in);
            if (op.suppress) {
                for (int i = 0; i < op.backspaces && !g_previewText.empty(); ++i) g_previewText.pop_back();
                g_previewText += op.insert;
            } else {
                g_previewText += static_cast<char32_t>(ch); // pass-through
            }
        } else {
            g_previewText += static_cast<char32_t>(ch);
        }
    }
    std::u16string u16 = unicode::utf32ToUtf16(g_previewText);
    SetWindowTextW(m_preview, std::wstring(reinterpret_cast<const wchar_t*>(u16.c_str()), u16.size()).c_str());
    SendMessageW(m_preview, EM_SETSEL, u16.size(), u16.size());
}

LRESULT CALLBACK DesignerWindow::PreviewProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                             UINT_PTR, DWORD_PTR ref)
{
    auto* self = reinterpret_cast<DesignerWindow*>(ref);
    if (msg == WM_CHAR && self) {
        self->FeedPreviewChar(static_cast<wchar_t>(wp));
        return 0; // consume; we manage the text ourselves
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

// ---- Window proc ------------------------------------------------------------

LRESULT CALLBACK DesignerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    DesignerWindow* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<DesignerWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<DesignerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self) return self->Handle(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT DesignerWindow::Handle(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case WM_SIZE:    Layout(); return 0;
        case WM_COMMAND: OnCommand(LOWORD(wp)); return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            RECT c = CanvasRect();
            HBRUSH bg = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &c, bg); DeleteObject(bg);
            PaintCanvas(hdc, c);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            unsigned vk = HitTestCanvas(LOWORD(lp), HIWORD(lp));
            if (vk) { m_selectedVk = vk; InvalidateRect(hwnd, nullptr, TRUE);
                      wchar_t s[64]; swprintf_s(s, L"Selected: %hs", vk::toName(vk).c_str());
                      SendMessageW(m_status, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(s)); }
            return 0;
        }
        case WM_LBUTTONDBLCLK: {
            unsigned vk = HitTestCanvas(LOWORD(lp), HIWORD(lp));
            if (vk) { m_selectedVk = vk; DoEditSelectedKey(); }
            return 0;
        }
        case WM_CLOSE:
            if (ConfirmDiscardIfDirty()) DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (m_capFont) DeleteObject(m_capFont);
            if (m_glyphFont) DeleteObject(m_glyphFont);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
