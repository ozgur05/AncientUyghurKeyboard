// Normalizer.hpp — bundled, dependency-free canonical normalization.
//
// SCOPE (read this): a full Unicode NFC implementation requires the entire
// Unicode Character Database. This project ships a *curated* table covering the
// Old Uyghur combining marks (U+10F82..U+10F85) plus the common Latin marks,
// which is exactly what this keyboard can produce. It performs:
//   1. canonical ordering  — stable sort of combining marks by combining class
//   2. canonical composition — starter + mark -> precomposed, via a bundled map
//
// This is correct for the characters in the bundled tables and leaves anything
// unknown untouched (never corrupts text). Extend the tables to widen coverage.
#pragma once

#include <string>
#include <cstdint>

namespace core::normalizer {

// Canonical Combining Class (0 = starter/base). Unknown -> 0.
int combiningClass(char32_t cp);

// Stable-sort combining marks within each starter run by combining class.
std::u32string canonicalOrder(const std::u32string& s);

// Compose starter+mark pairs found in the bundled table (input assumed ordered).
std::u32string compose(const std::u32string& s);

// NFC = canonicalOrder then compose.
std::u32string toNFC(const std::u32string& s);

} // namespace core::normalizer
