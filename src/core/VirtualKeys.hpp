// VirtualKeys.hpp — portable virtual-key names <-> codes.
//
// The numeric values match the Win32 VK_* codes exactly (the values delivered
// by KBDLLHOOKSTRUCT::vkCode), but this header intentionally does NOT include
// <windows.h>, so the layout core and its tests stay platform-independent.
//
// Layout JSON files address keys by name ("A", "Space", "OEM_1", "0xBA"),
// which we resolve to codes here.
#pragma once

#include <string>
#include <cctype>
#include <cstdint>
#include <optional>

namespace core::vk {

// Codes identical to the Win32 VK_* set we care about.
enum : unsigned {
    Back      = 0x08, // Backspace
    Tab       = 0x09,
    Enter     = 0x0D,
    Shift     = 0x10,
    Control   = 0x11,
    Menu      = 0x12, // Alt
    Capital   = 0x14, // Caps Lock
    Escape    = 0x1B,
    Space     = 0x20,

    Key0 = 0x30, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
    KeyA = 0x41, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ,
    KeyK, KeyL, KeyM, KeyN, KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT,
    KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ,

    LWin = 0x5B, RWin = 0x5C,

    Numpad0 = 0x60, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    Multiply = 0x6A, Add = 0x6B, Separator = 0x6C,
    Subtract = 0x6D, Decimal = 0x6E, Divide = 0x6F,

    // OEM keys on a US layout:
    OEM_1      = 0xBA, // ;:
    OEM_Plus   = 0xBB, // =+
    OEM_Comma  = 0xBC, // ,<
    OEM_Minus  = 0xBD, // -_
    OEM_Period = 0xBE, // .>
    OEM_2      = 0xBF, // /?
    OEM_3      = 0xC0, // `~
    OEM_4      = 0xDB, // [{
    OEM_5      = 0xDC, // backslash |
    OEM_6      = 0xDD, // ]}
    OEM_7      = 0xDE, // '"
};

// Parse "0x1B" / "27" numeric spellings. Returns nullopt on failure.
inline std::optional<unsigned> parseNumeric(const std::string& s)
{
    if (s.empty()) return std::nullopt;
    try {
        size_t idx = 0;
        unsigned long v;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            v = std::stoul(s.substr(2), &idx, 16), idx += 2;
        else
            v = std::stoul(s, &idx, 10);
        if (idx != s.size()) return std::nullopt;
        return static_cast<unsigned>(v);
    } catch (...) {
        return std::nullopt;
    }
}

// Resolve a key name from a layout file to a VK code.
// Accepts: single chars A-Z / 0-9, friendly names, "OEM_1"-style, and numerics.
inline std::optional<unsigned> fromName(const std::string& raw)
{
    if (raw.empty()) return std::nullopt;

    // Uppercase copy for case-insensitive matching.
    std::string s;
    s.reserve(raw.size());
    for (char c : raw) s += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    // Single character letter/digit.
    if (s.size() == 1) {
        char c = s[0];
        if (c >= 'A' && c <= 'Z') return static_cast<unsigned>(KeyA + (c - 'A'));
        if (c >= '0' && c <= '9') return static_cast<unsigned>(Key0 + (c - '0'));
    }

    // Friendly names.
    if (s == "SPACE")     return Space;
    if (s == "ENTER" || s == "RETURN") return Enter;
    if (s == "TAB")       return Tab;
    if (s == "BACKSPACE" || s == "BACK") return Back;
    if (s == "ESC" || s == "ESCAPE")     return Escape;
    if (s == "CAPSLOCK" || s == "CAPITAL") return Capital;

    // OEM_n style.
    if (s == "OEM_1")      return OEM_1;
    if (s == "OEM_2")      return OEM_2;
    if (s == "OEM_3")      return OEM_3;
    if (s == "OEM_4")      return OEM_4;
    if (s == "OEM_5")      return OEM_5;
    if (s == "OEM_6")      return OEM_6;
    if (s == "OEM_7")      return OEM_7;
    if (s == "OEM_PLUS")   return OEM_Plus;
    if (s == "OEM_MINUS")  return OEM_Minus;
    if (s == "OEM_COMMA")  return OEM_Comma;
    if (s == "OEM_PERIOD") return OEM_Period;

    // Named punctuation aliases (nicer to read in JSON).
    if (s == "SEMICOLON")    return OEM_1;
    if (s == "SLASH")        return OEM_2;
    if (s == "BACKTICK" || s == "GRAVE") return OEM_3;
    if (s == "LBRACKET")     return OEM_4;
    if (s == "BACKSLASH")    return OEM_5;
    if (s == "RBRACKET")     return OEM_6;
    if (s == "QUOTE" || s == "APOSTROPHE") return OEM_7;
    if (s == "EQUALS" || s == "PLUS")      return OEM_Plus;
    if (s == "MINUS" || s == "DASH")       return OEM_Minus;
    if (s == "COMMA")        return OEM_Comma;
    if (s == "PERIOD" || s == "DOT")       return OEM_Period;

    // Numpad.
    if (s.rfind("NUMPAD", 0) == 0 && s.size() == 7 && s[6] >= '0' && s[6] <= '9')
        return static_cast<unsigned>(Numpad0 + (s[6] - '0'));

    // Numeric fallback ("0xBA", "186").
    return parseNumeric(raw);
}

// Is this VK an ASCII letter key (A-Z)? Used for Caps Lock logic.
inline bool isLetter(unsigned vk) { return vk >= KeyA && vk <= KeyZ; }

} // namespace core::vk
