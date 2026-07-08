#include "UnicodePicker.h"
#include "Modal.h"
#include "../core/UnicodeNames.hpp"
#include "../core/Unicode.hpp"

#include <vector>
#include <string>

namespace {

constexpr int ID_SEARCH  = 1001;
constexpr int ID_LIST    = 1002;
constexpr int ID_PREVIEW = 1003;
constexpr int ID_OK      = IDOK;      // 1
constexpr int ID_CANCEL  = IDCANCEL;  // 2

struct PickerState {
    bool                    done = false;
    bool                    accepted = false;
    char32_t                result = 0;
    std::vector<char32_t>   items;      // parallels the listbox rows
    HWND                    hSearch = nullptr;
    HWND                    hList = nullptr;
    HWND                    hPreview = nullptr;
    HFONT                   previewFont = nullptr;
};

std::wstring toW(const std::u32string& s)
{
    std::u16string u16 = core::unicode::utf32ToUtf16(s);
    return std::wstring(reinterpret_cast<const wchar_t*>(u16.c_str()), u16.size());
}
std::wstring toW(const std::string& s) // ASCII/UTF-8 -> wide
{
    std::u32string u32 = core::unicode::utf8ToUtf32(s);
    return toW(u32);
}

void populate(PickerState* st, const std::wstring& query)
{
    // Convert wide query to UTF-8 for the core search.
    std::u16string q16(query.begin(), query.end());
    std::string q = core::unicode::utf32ToUtf8(core::unicode::utf16ToUtf32(q16));

    auto results = core::UnicodeNames::search(q, 500);
    SendMessageW(st->hList, LB_RESETCONTENT, 0, 0);
    st->items.clear();
    wchar_t line[160];
    for (const auto& r : results) {
        swprintf_s(line, L"U+%04X   %hs", static_cast<unsigned>(r.cp), r.name.c_str());
        SendMessageW(st->hList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line));
        st->items.push_back(r.cp);
    }
    if (!st->items.empty())
        SendMessageW(st->hList, LB_SETCURSEL, 0, 0);
}

void updatePreview(PickerState* st)
{
    int sel = static_cast<int>(SendMessageW(st->hList, LB_GETCURSEL, 0, 0));
    if (sel < 0 || sel >= static_cast<int>(st->items.size())) {
        SetWindowTextW(st->hPreview, L"");
        return;
    }
    SetWindowTextW(st->hPreview, toW(std::u32string(1, st->items[sel])).c_str());
}

LRESULT CALLBACK PickerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* st = reinterpret_cast<PickerState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (id == ID_SEARCH && HIWORD(wp) == EN_CHANGE && st) {
                wchar_t buf[128] = {0};
                GetWindowTextW(st->hSearch, buf, 127);
                populate(st, buf);
                updatePreview(st);
                return 0;
            }
            if (id == ID_LIST && HIWORD(wp) == LBN_SELCHANGE && st) { updatePreview(st); return 0; }
            if (id == ID_OK || (id == ID_LIST && HIWORD(wp) == LBN_DBLCLK)) {
                if (st) {
                    int sel = static_cast<int>(SendMessageW(st->hList, LB_GETCURSEL, 0, 0));
                    if (sel >= 0 && sel < static_cast<int>(st->items.size())) {
                        st->result = st->items[sel];
                        st->accepted = true;
                        st->done = true;
                    }
                }
                return 0;
            }
            if (id == ID_CANCEL && st) { st->done = true; return 0; }
            break;
        }
        case WM_CLOSE:
            if (st) st->done = true;
            return 0;
        case WM_DESTROY:
            if (st && st->previewFont) { DeleteObject(st->previewFont); st->previewFont = nullptr; }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

std::optional<char32_t> PickUnicode(HWND owner, HINSTANCE hInst)
{
    static bool registered = false;
    const wchar_t* kClass = L"AUKUnicodePicker";
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc   = PickerProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = kClass;
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        RegisterClassExW(&wc);
        registered = true;
    }

    PickerState st;
    RECT r = { 0, 0, 460, 380 };
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, kClass, L"Insert Unicode Character",
        WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right, r.bottom, owner, nullptr, hInst, nullptr);
    if (!dlg) return std::nullopt;
    SetWindowLongPtrW(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&st));

    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
            x, y, w, h, dlg, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, nullptr);
    };

    mk(L"STATIC", L"Search (name or U+XXXX):", 0, 12, 12, 240, 18, -1);
    st.hSearch  = mk(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 12, 32, 300, 24, ID_SEARCH);
    st.hList    = mk(L"LISTBOX", L"", WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 12, 64, 300, 270, ID_LIST);
    mk(L"STATIC", L"Preview:", 0, 324, 64, 120, 18, -1);
    st.hPreview = mk(L"STATIC", L"", SS_CENTER | WS_BORDER, 324, 84, 120, 90, ID_PREVIEW);
    mk(L"BUTTON", L"Insert", BS_DEFPUSHBUTTON, 324, 260, 120, 30, ID_OK);
    mk(L"BUTTON", L"Cancel", 0, 324, 300, 120, 30, ID_CANCEL);

    // Large font for the preview glyph.
    st.previewFont = CreateFontW(56, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(st.hPreview, WM_SETFONT, reinterpret_cast<WPARAM>(st.previewFont), TRUE);

    populate(&st, L"");
    updatePreview(&st);
    SetFocus(st.hSearch);

    RunModal(owner, dlg, st.done);
    if (st.accepted) return st.result;
    return std::nullopt;
}
