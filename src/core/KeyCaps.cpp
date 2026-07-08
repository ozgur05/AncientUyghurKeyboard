#include "KeyCaps.hpp"
#include "VirtualKeys.hpp"

#include <algorithm>

namespace core {

namespace {

// Helper builders for readability.
KeyCap edit(double x, double y, double w, unsigned vk, const char* label)
{
    return KeyCap{ x, y, w, 1.0, vk, label, true };
}
KeyCap deco(double x, double y, double w, const char* label)
{
    return KeyCap{ x, y, w, 1.0, 0, label, false };
}

// The alphanumeric block shared by ANSI and ISO (rows 0..3 + space row 4).
// Differences: ANSI has a wide Enter on row 2 and a 2.0-wide LShift start on
// row 3; ISO has an L-shaped Enter (row1 tall) + extra OEM key, and a split
// LShift with OEM_5 (\|) beside it.
void addCommonRows(std::vector<KeyCap>& v, BoardType type)
{
    using namespace vk;

    // Row 0 — number row.
    double x = 0;
    v.push_back(edit(x, 0, 1.0, OEM_3, "`"));  x += 1.0;
    const char* nums[] = { "1","2","3","4","5","6","7","8","9","0" };
    unsigned numvk[]   = { Key1,Key2,Key3,Key4,Key5,Key6,Key7,Key8,Key9,Key0 };
    for (int i = 0; i < 10; ++i) { v.push_back(edit(x, 0, 1.0, numvk[i], nums[i])); x += 1.0; }
    v.push_back(edit(x, 0, 1.0, OEM_Minus, "-")); x += 1.0;
    v.push_back(edit(x, 0, 1.0, OEM_Plus,  "=")); x += 1.0;
    v.push_back(deco(x, 0, 2.0, "Backspace"));

    // Row 1 — QWERTY.
    x = 0;
    v.push_back(deco(x, 1, 1.5, "Tab")); x += 1.5;
    const char* r1[] = { "Q","W","E","R","T","Y","U","I","O","P" };
    unsigned r1vk[]  = { KeyQ,KeyW,KeyE,KeyR,KeyT,KeyY,KeyU,KeyI,KeyO,KeyP };
    for (int i = 0; i < 10; ++i) { v.push_back(edit(x, 1, 1.0, r1vk[i], r1[i])); x += 1.0; }
    v.push_back(edit(x, 1, 1.0, OEM_4, "[")); x += 1.0;
    v.push_back(edit(x, 1, 1.0, OEM_6, "]")); x += 1.0;
    if (type == BoardType::ANSI) {
        v.push_back(edit(x, 1, 1.5, OEM_5, "\\")); // ANSI backslash on row 1
    }
    // ISO: the \ key moves to row 3; row 1 tail is filled by the tall Enter.

    // Row 2 — home row.
    x = 0;
    v.push_back(deco(x, 2, 1.75, "Caps")); x += 1.75;
    const char* r2[] = { "A","S","D","F","G","H","J","K","L" };
    unsigned r2vk[]  = { KeyA,KeyS,KeyD,KeyF,KeyG,KeyH,KeyJ,KeyK,KeyL };
    for (int i = 0; i < 9; ++i) { v.push_back(edit(x, 2, 1.0, r2vk[i], r2[i])); x += 1.0; }
    v.push_back(edit(x, 2, 1.0, OEM_1, ";")); x += 1.0;
    v.push_back(edit(x, 2, 1.0, OEM_7, "'")); x += 1.0;
    if (type == BoardType::ANSI) {
        v.push_back(deco(x, 2, 2.25, "Enter"));
    } else {
        // ISO adds OEM_5 here and uses the L-shaped Enter (drawn on rows 1-2).
        v.push_back(edit(x, 2, 1.0, OEM_5, "#")); x += 1.0;
        v.push_back(deco(13.75, 1, 1.5, "Enter")); // tall enter top segment
        v.push_back(deco(x, 2, 1.25, ""));          // enter bottom segment
    }

    // Row 3 — bottom letter row.
    x = 0;
    if (type == BoardType::ANSI) {
        v.push_back(deco(x, 3, 2.25, "Shift")); x += 2.25;
    } else {
        v.push_back(deco(x, 3, 1.25, "Shift")); x += 1.25;
        v.push_back(edit(x, 3, 1.0, OEM_5, "\\")); x += 1.0; // ISO extra key by LShift
    }
    const char* r3[] = { "Z","X","C","V","B","N","M" };
    unsigned r3vk[]  = { KeyZ,KeyX,KeyC,KeyV,KeyB,KeyN,KeyM };
    for (int i = 0; i < 7; ++i) { v.push_back(edit(x, 3, 1.0, r3vk[i], r3[i])); x += 1.0; }
    v.push_back(edit(x, 3, 1.0, OEM_Comma,  ",")); x += 1.0;
    v.push_back(edit(x, 3, 1.0, OEM_Period, ".")); x += 1.0;
    v.push_back(edit(x, 3, 1.0, OEM_2,      "/")); x += 1.0;
    v.push_back(deco(x, 3, 2.75, "Shift"));

    // Row 4 — space row (modifiers + space; space is editable as a key).
    x = 0;
    v.push_back(deco(x, 4, 1.25, "Ctrl"));  x += 1.25;
    v.push_back(deco(x, 4, 1.25, "Win"));   x += 1.25;
    v.push_back(deco(x, 4, 1.25, "Alt"));   x += 1.25;
    v.push_back(edit(x, 4, 6.25, vk::Space, "Space")); x += 6.25;
    v.push_back(deco(x, 4, 1.25, "AltGr")); x += 1.25;
    v.push_back(deco(x, 4, 1.25, "Win"));   x += 1.25;
    v.push_back(deco(x, 4, 1.25, "Menu"));  x += 1.25;
    v.push_back(deco(x, 4, 1.25, "Ctrl"));
}

const std::vector<KeyCap>& ansi()
{
    static const std::vector<KeyCap> v = [] {
        std::vector<KeyCap> caps;
        addCommonRows(caps, BoardType::ANSI);
        return caps;
    }();
    return v;
}

const std::vector<KeyCap>& iso()
{
    static const std::vector<KeyCap> v = [] {
        std::vector<KeyCap> caps;
        addCommonRows(caps, BoardType::ISO);
        return caps;
    }();
    return v;
}

double computeWidth(const std::vector<KeyCap>& v)
{
    double w = 0;
    for (const auto& c : v) w = std::max(w, c.x + c.w);
    return w;
}
double computeHeight(const std::vector<KeyCap>& v)
{
    double h = 0;
    for (const auto& c : v) h = std::max(h, c.y + c.h);
    return h;
}

} // namespace

const std::vector<KeyCap>& KeyCaps::caps(BoardType type)
{
    return type == BoardType::ISO ? iso() : ansi();
}

double KeyCaps::width(BoardType type)  { return computeWidth(caps(type)); }
double KeyCaps::height(BoardType type) { return computeHeight(caps(type)); }

} // namespace core
