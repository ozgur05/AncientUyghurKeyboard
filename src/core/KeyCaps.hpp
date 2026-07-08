// KeyCaps.hpp — physical keyboard geometry for the visual editor.
//
// Describes the position and size of each key cap for the ANSI and ISO layouts,
// in abstract grid units (1.0 == one standard key width, rows are 1.0 tall).
// The designer canvas scales these to pixels. Each cap carries the VK code the
// editor edits when the cap is clicked. Pure data, portable, unit-tested.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace core {

enum class BoardType { ANSI, ISO };

struct KeyCap {
    double      x = 0;      // grid units from the left of the row area
    double      y = 0;      // row index (0 = number row)
    double      w = 1.0;    // width in key units
    double      h = 1.0;    // height in key units
    unsigned    vk = 0;     // virtual-key this cap edits (0 = decorative/non-editable)
    std::string label;      // face legend (e.g. "A", "Tab", "Shift")
    bool        editable = true; // false for pure modifiers we don't remap
};

class KeyCaps {
public:
    // The caps for a board type, in a stable order (top-left to bottom-right).
    static const std::vector<KeyCap>& caps(BoardType type);

    // Total board width/height in key units (for canvas scaling).
    static double width(BoardType type);
    static double height(BoardType type);
};

} // namespace core
