#include "KeyEditDialog.h"
#include "Modal.h"
#include "UnicodePicker.h"
#include "../core/Unicode.hpp"
#include "../core/VirtualKeys.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <cctype>

using core::KeyAction;
using core::KeyDef;
using core::ActionKind;
using core::Level;

namespace {

constexpr int kLevels = 4;
const wchar_t* kLevelLabels[kLevels] = { L"Base", L"Shift", L"AltGr", L"Shift+AltGr" };

// Control id scheme: 3000 + level*10 + field (0=combo,1=edit,2=pick).
int idFor(int level, int field) { return 3000 + level * 10 + field; }

std::wstring toW(const std::string& s)
{
    std::u32string u32 = core::unicode::utf8ToUtf32(s);
    std::u16string u16 = core::unicode::utf32ToUtf16(u32);
    return std::wstring(reinterpret_cast<const wchar_t*>(u16.c_str()), u16.size());
}
std::string toUtf8(const std::wstring& w)
{
    std::u16string u16(w.begin(), w.end());
    return core::unicode::utf32ToUtf8(core::unicode::utf16ToUtf32(u16));
}

std::wstring cpToken(char32_t cp)
{
    wchar_t b[16];
    swprintf_s(b, L"U+%04X", static_cast<unsigned>(cp));
    return b;
}
std::wstring tokensOf(const std::u32string& s)
{
    std::wstring out;
    for (size_t i = 0; i < s.size(); ++i) { if (i) out += L' '; out += cpToken(s[i]); }
    return out;
}

// Parse "U+XXXX 0xYYYY 65" into codepoints; invalid/surrogate tokens are dropped.
std::u32string parseTokens(const std::wstring& text)
{
    std::u32string out;
    std::wstringstream ss(text);
    std::wstring tok;
    while (ss >> tok) {
        std::wstring t = tok;
        int base = 10;
        if (t.size() > 2 && (t[0] == L'U' || t[0] == L'u') && t[1] == L'+') { t = t.substr(2); base = 16; }
        else if (t.size() > 2 && t[0] == L'0' && (t[1] == L'x' || t[1] == L'X')) { t = t.substr(2); base = 16; }
        try {
            unsigned long v = std::stoul(t, nullptr, base);
            if (v <= 0x10FFFF && core::unicode::isValidScalar(static_cast<char32_t>(v)))
                out += static_cast<char32_t>(v);
        } catch (...) { /* skip */ }
    }
    return out;
}

struct EditState {
    bool     done = false;
    bool     accepted = false;
    unsigned vk = 0;
    HWND     combo[kLevels] = {};
    HWND     edit[kLevels]  = {};
    HINSTANCE hInst = nullptr;
    KeyDef   result;
};

void applyResult(EditState* st)
{
    KeyDef def;
    def.cased = core::vk::isLetter(st->vk);
    for (int i = 0; i < kLevels; ++i) {
        int type = static_cast<int>(SendMessageW(st->combo[i], CB_GETCURSEL, 0, 0));
        wchar_t buf[512] = {0};
        GetWindowTextW(st->edit[i], buf, 511);
        KeyAction a;
        switch (type) {
            case 1: a.kind = ActionKind::Emit;    a.output = parseTokens(buf); break;
            case 2: a.kind = ActionKind::DeadKey; a.deadKey = toUtf8(buf);     break;
            case 3: a.kind = ActionKind::Compose;                             break;
            default: a.kind = ActionKind::None; break;
        }
        if (a.kind == ActionKind::Emit && def.cased) a.cased = true;
        if (a.kind == ActionKind::Emit && a.output.empty()) a.kind = ActionKind::None;
        def.levels[i] = a;
    }
    st->result = def;
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* st = reinterpret_cast<EditState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            if (st && id >= 3000 && id < 3000 + kLevels * 10) {
                int level = (id - 3000) / 10;
                int field = (id - 3000) % 10;
                if (field == 2 && HIWORD(wp) == BN_CLICKED) { // Pick...
                    if (auto cp = PickUnicode(hwnd, st->hInst)) {
                        // Append the token to the edit field and ensure Emit type.
                        wchar_t buf[512] = {0};
                        GetWindowTextW(st->edit[level], buf, 511);
                        std::wstring cur = buf;
                        if (!cur.empty() && cur.back() != L' ') cur += L' ';
                        cur += cpToken(*cp);
                        SetWindowTextW(st->edit[level], cur.c_str());
                        SendMessageW(st->combo[level], CB_SETCURSEL, 1, 0); // Emit
                    }
                    return 0;
                }
            }
            if (id == IDOK && st)     { applyResult(st); st->accepted = true; st->done = true; return 0; }
            if (id == IDCANCEL && st) { st->done = true; return 0; }
            break;
        }
        case WM_CLOSE: if (st) st->done = true; return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

bool EditKey(HWND owner, HINSTANCE hInst, unsigned vk, KeyDef& def)
{
    static bool registered = false;
    const wchar_t* kClass = L"AUKKeyEditDialog";
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc   = EditProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = kClass;
        wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        RegisterClassExW(&wc);
        registered = true;
    }

    EditState st;
    st.vk = vk;
    st.hInst = hInst;

    wchar_t title[64];
    swprintf_s(title, L"Edit key: %hs", core::vk::toName(vk).c_str());

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, kClass, title,
        WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        560, 260, owner, nullptr, hInst, nullptr);
    if (!dlg) return false;
    SetWindowLongPtrW(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&st));

    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                  int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h,
            dlg, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, nullptr);
    };

    for (int i = 0; i < kLevels; ++i) {
        int y = 12 + i * 44;
        mk(L"STATIC", kLevelLabels[i], 0, 12, y + 4, 80, 20, -1);
        HWND cb = mk(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 96, y, 110, 200, idFor(i, 0));
        for (const wchar_t* opt : { L"(none)", L"Emit", L"Dead key", L"Compose" })
            SendMessageW(cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(opt));
        HWND ed = mk(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 214, y, 220, 24, idFor(i, 1));
        mk(L"BUTTON", L"Pick…", 0, 444, y, 90, 24, idFor(i, 2));
        st.combo[i] = cb;
        st.edit[i]  = ed;

        // Seed from the incoming KeyDef.
        const KeyAction& a = def.levels[i];
        int sel = 0; std::wstring val;
        switch (a.kind) {
            case ActionKind::Emit:    sel = 1; val = tokensOf(a.output); break;
            case ActionKind::DeadKey: sel = 2; val = toW(a.deadKey);     break;
            case ActionKind::Compose: sel = 3; break;
            default: sel = 0; break;
        }
        SendMessageW(cb, CB_SETCURSEL, sel, 0);
        SetWindowTextW(ed, val.c_str());
    }

    mk(L"BUTTON", L"OK",     BS_DEFPUSHBUTTON, 350, 200, 90, 28, IDOK);
    mk(L"BUTTON", L"Cancel", 0,                444, 200, 90, 28, IDCANCEL);

    RunModal(owner, dlg, st.done);
    if (st.accepted) { def = st.result; return true; }
    return false;
}
