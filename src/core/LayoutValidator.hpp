// LayoutValidator.hpp — semantic validation of a built KeyboardLayout.
//
// The parser guarantees structural soundness; the validator checks meaning:
//   * every DeadKey action references a dead key that is actually defined
//   * dead keys / compose sequences aren't degenerate
//   * compose sequences aren't duplicated or ambiguous (an exact match that is
//     also a proper prefix of a longer sequence can never fire the longer one)
//   * two keys producing the same glyph (reported as a warning)
// Errors mean "do not activate this layout"; warnings are advisory.
#pragma once

#include "KeyboardLayout.hpp"
#include <string>
#include <vector>

namespace core {

struct ValidationReport {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    bool ok() const { return errors.empty(); }
};

class LayoutValidator {
public:
    static ValidationReport validate(const KeyboardLayout& layout);
};

} // namespace core
