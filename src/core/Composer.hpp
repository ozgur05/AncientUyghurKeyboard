// Composer.hpp — stateful text composition over a KeyboardLayout.
//
// Turns physical key events into edit operations: how many Backspaces to send
// and which codepoints to insert. It owns the dead-key state machine, ligature
// collapsing, and NFC reconciliation.
//
// RECONCILIATION MODEL (and its honest limits): because a hook-based keyboard
// cannot read the target application's buffer, the Composer keeps its own
// window of recently emitted codepoints. When normalization or a ligature must
// rewrite text that was already emitted, it emits Backspaces to delete the
// changed tail and re-emits the corrected form. This is best-effort and
// editor-dependent (a Backspace is assumed to delete exactly one codepoint).
// The window resets on word boundaries and on reset().
#pragma once

#include "KeyboardLayout.hpp"
#include <string>

namespace core {

struct KeyInput {
    unsigned vk    = 0;
    bool     shift = false;
    bool     altgr = false;
    bool     caps  = false;
};

struct EmitOp {
    bool           suppress   = false; // true => swallow the original key event
    int            backspaces = 0;     // Backspaces to send before `insert`
    std::u32string insert;             // codepoints to inject

    bool empty() const { return backspaces == 0 && insert.empty(); }
};

class Composer {
public:
    void setLayout(const KeyboardLayout* layout) { m_layout = layout; reset(); }
    const KeyboardLayout* layout() const { return m_layout; }

    // Process one key-down. Returns the edit to apply.
    EmitOp process(const KeyInput& in);

    // Clear all composition state (call on focus change / layout reload).
    void reset();

    // Exposed for tests.
    const std::u32string& window() const { return m_recent; }
    bool deadPending() const { return !m_pendingDead.empty(); }

private:
    EmitOp commit(const std::u32string& produced);
    std::u32string applyLigatures(std::u32string text) const;

    static constexpr size_t kWindow = 32;

    const KeyboardLayout* m_layout = nullptr;
    std::u32string        m_recent;       // on-screen tail we believe we emitted
    std::string           m_pendingDead;  // active dead-key id, empty if none
};

} // namespace core
