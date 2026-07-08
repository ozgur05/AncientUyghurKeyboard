// LayoutSerializer.hpp — KeyboardLayout -> JSON text.
//
// The inverse of LayoutParser: produces a UTF-8 JSON document that, when read
// back by LayoutParser, yields an equivalent layout (round-trip stable). This
// is what the designer's "Save as JSON" / "Export" use. Pure and portable, so
// the round-trip is unit-tested without Windows.
#pragma once

#include "KeyboardLayout.hpp"
#include <string>

namespace core {

class LayoutSerializer {
public:
    // Serialize `layout` to a pretty-printed JSON string (2-space indent).
    // Codepoints are written as "U+XXXX" tokens for stability and readability.
    static std::string toJson(const KeyboardLayout& layout);
};

} // namespace core
