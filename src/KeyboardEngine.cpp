#include "KeyboardEngine.h"
#include "Logger.h"
#include "core/Unicode.hpp"
#include "core/VirtualKeys.hpp"
#include "core/Stopwatch.hpp"

#include <vector>
#include <cstdlib>

KeyboardEngine* KeyboardEngine::s_instance = nullptr;

KeyboardEngine::KeyboardEngine()
{
    s_instance = this;
    // Opt-in per-keystroke hook timing: set AUK_DIAG=1 in the environment.
    size_t len = 0;
    char val[8] = { 0 };
    if (getenv_s(&len, val, sizeof(val), "AUK_DIAG") == 0 && len > 0 && val[0] == '1')
        m_diag = true;
}

KeyboardEngine::~KeyboardEngine()
{
    Uninstall();
    if (s_instance == this)
        s_instance = nullptr;
}

void KeyboardEngine::SetLayout(const core::KeyboardLayout* layout)
{
    m_composer.setLayout(layout);
}

bool KeyboardEngine::Install()
{
    if (m_hook)
        return true;
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, &KeyboardEngine::HookProc,
                               GetModuleHandleW(nullptr), 0);
    if (!m_hook) {
        Logger::Instance().Error(L"SetWindowsHookExW failed");
        return false;
    }
    Logger::Instance().Info(L"Keyboard hook installed");
    return true;
}

void KeyboardEngine::Uninstall()
{
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
        Logger::Instance().Info(L"Keyboard hook removed");
    }
}

LRESULT CALLBACK KeyboardEngine::HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION && s_instance) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            const auto* info = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
            if (info) {
                bool consumed;
                if (s_instance->m_diag) {
                    core::Stopwatch sw;
                    consumed = s_instance->HandleKeyDown(*info);
                    const double us = sw.micros();
                    if (us > 500.0) { // only log unusually slow handling
                        wchar_t buf[64];
                        swprintf_s(buf, L"Hook handled key in %.1f us", us);
                        Logger::Instance().Trace(buf);
                    }
                } else {
                    consumed = s_instance->HandleKeyDown(*info);
                }
                if (consumed)
                    return 1; // consumed — suppress the original key
            }
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

bool KeyboardEngine::HandleKeyDown(const KBDLLHOOKSTRUCT& info)
{
    // Ignore input we injected ourselves.
    if (m_injecting || (info.flags & LLKHF_INJECTED))
        return false;
    if (!m_enabled || !m_composer.layout())
        return false;

    // Never route pure modifier key events to the composer. The LL hook
    // delivers side-specific codes (VK_LSHIFT/VK_RSHIFT/...), so check those
    // too — otherwise a Shift press would clear the composition window.
    switch (info.vkCode) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU:
        case VK_LWIN: case VK_RWIN:
        case VK_CAPITAL:
            return false;
        default:
            break;
    }

    auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };

    const bool shift = down(VK_SHIFT);
    const bool ctrl  = down(VK_CONTROL);
    const bool lAlt  = down(VK_LMENU);
    const bool rAlt  = down(VK_RMENU);
    const bool win   = down(VK_LWIN) || down(VK_RWIN);
    const bool caps  = (GetKeyState(VK_CAPITAL) & 1) != 0;

    // Treat Right Alt alone (for US QWERTY Windows users) OR Ctrl+LeftAlt as AltGr.
    const bool altgr = rAlt || (ctrl && lAlt);

    // Let genuine shortcuts (Win+_, LeftAlt+_, Ctrl+_) pass straight through.
    if (win) { m_composer.reset(); return false; }
    if (lAlt && !altgr) { m_composer.reset(); return false; }
    if (ctrl && !altgr) { m_composer.reset(); return false; }

    core::KeyInput in;
    in.vk    = info.vkCode;
    in.shift = shift;
    in.altgr = altgr;
    in.caps  = caps;

    core::EmitOp op = m_composer.process(in);
    if (!op.empty())
        SendEdits(op);
    return op.suppress;
}

void KeyboardEngine::SendEdits(const core::EmitOp& op)
{
    std::vector<INPUT> inputs;
    inputs.reserve(static_cast<size_t>(op.backspaces) * 2 + op.insert.size() * 2 + 8);

    // --- Temporarily release held Ctrl/Alt modifiers so apps like LibreOffice
    //     don't misinterpret the injected Unicode chars as shortcut combos. ---
    struct HeldMod { int vk; DWORD flags; };
    HeldMod modifiers[] = {
        { VK_LCONTROL, 0 },
        { VK_RCONTROL, KEYEVENTF_EXTENDEDKEY },
        { VK_LMENU,    0 },
        { VK_RMENU,    KEYEVENTF_EXTENDEDKEY },
    };
    std::vector<HeldMod> heldMods;
    for (const auto& mod : modifiers) {
        if ((GetAsyncKeyState(mod.vk) & 0x8000) != 0) {
            heldMods.push_back(mod);
            INPUT up{}; up.type = INPUT_KEYBOARD;
            up.ki.wVk = static_cast<WORD>(mod.vk);
            up.ki.dwFlags = KEYEVENTF_KEYUP | mod.flags;
            inputs.push_back(up);
        }
    }

    // Backspaces first (delete the stale tail during reconciliation).
    for (int i = 0; i < op.backspaces; ++i) {
        INPUT d{}; d.type = INPUT_KEYBOARD; d.ki.wVk = VK_BACK;
        inputs.push_back(d);
        INPUT u{}; u.type = INPUT_KEYBOARD; u.ki.wVk = VK_BACK; u.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(u);
    }

    // Insert codepoints as UTF-16 Unicode input.
    std::u16string u16 = core::unicode::utf32ToUtf16(op.insert);
    for (char16_t ch : u16) {
        INPUT d{}; d.type = INPUT_KEYBOARD; d.ki.wScan = ch; d.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(d);
        INPUT u{}; u.type = INPUT_KEYBOARD; u.ki.wScan = ch;
        u.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(u);
    }

    // --- Re-press any modifiers we released so holding Right Alt keeps working. ---
    for (const auto& mod : heldMods) {
        INPUT dn{}; dn.type = INPUT_KEYBOARD;
        dn.ki.wVk = static_cast<WORD>(mod.vk);
        dn.ki.dwFlags = mod.flags;
        inputs.push_back(dn);
    }

    if (!inputs.empty()) {
        m_injecting = true;
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
        m_injecting = false;
    }
}
