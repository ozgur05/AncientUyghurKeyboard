// KeyboardEngine.h — global low-level keyboard hook driving the Composer.
//
// Installs a WH_KEYBOARD_LL hook. Each key-down is translated into a KeyInput
// (with live Shift / AltGr / Caps state) and handed to the Composer, which
// returns an EmitOp describing Backspaces + codepoints to inject via SendInput.
#pragma once

#include <windows.h>
#include "core/Composer.hpp"
#include "core/KeyboardLayout.hpp"

class KeyboardEngine {
public:
    KeyboardEngine();
    ~KeyboardEngine();

    bool Install();
    void Uninstall();

    // Point the composer at a layout (nullptr disables mapping). The layout
    // must outlive the engine (Application owns it). Also resets composition.
    void SetLayout(const core::KeyboardLayout* layout);

    void Enable(bool on) { m_enabled = on; if (!on) m_composer.reset(); }
    bool Enabled() const { return m_enabled; }

    void ResetComposition() { m_composer.reset(); }

    KeyboardEngine(const KeyboardEngine&) = delete;
    KeyboardEngine& operator=(const KeyboardEngine&) = delete;

private:
    static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
    bool HandleKeyDown(const KBDLLHOOKSTRUCT& info);
    void SendEdits(const core::EmitOp& op);

    static KeyboardEngine* s_instance;

    HHOOK          m_hook      = nullptr;
    bool           m_enabled   = true;
    bool           m_injecting = false;   // re-entrancy guard for injected input
    bool           m_diag      = false;   // hook timing diagnostics (env AUK_DIAG)
    core::Composer m_composer;
};
