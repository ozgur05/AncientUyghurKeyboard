// UnicodePicker.h — modal Unicode code-point picker dialog.
#pragma once

#include <windows.h>
#include <optional>
#include <cstdint>

// Show a modal picker. Returns the chosen codepoint, or nullopt if cancelled.
// Search accepts a character-name substring or a codepoint spelling
// ("U+10F70" / "0x10F70" / "10F70"). SMP code points are fully supported.
std::optional<char32_t> PickUnicode(HWND owner, HINSTANCE hInst);
