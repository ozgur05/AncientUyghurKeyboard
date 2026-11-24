# Release Notes — v1.1.0

**Ancient Uyghur Keyboard 1.1.0** — Complete glyph coverage for QochoOldUyghur
and other Old Uyghur OpenType fonts.

## Highlights

- **Full font coverage.** Both bundled layouts now expose every typable codepoint
  in QochoOldUyghur-Regular v1.000, NotoSerifOldUyghur, and Ugurica — including
  the text-shaping controls (ZWJ, ZWNJ, Tatweel) that unlock the font's 109
  glyphs (positional forms, ligatures, combining marks).
- **Zero Width Joiner / Non-Joiner on the keyboard.** `AltGr+=` inserts ZWJ
  (U+200D) to force letter joining; `AltGr+-` inserts ZWNJ (U+200C) to break
  it. These are essential for controlling which of the font's `.init`, `.medi`,
  `.fina` forms appear.
- **Tatweel (kashida).** `AltGr+/` inserts the Arabic Tatweel (U+0640) for
  elongation between joined letters.
- **5 ligature compose shortcuts.** `AltGr+/` followed by two letter keys
  produces the codepoint pairs that trigger the font's OpenType ligatures
  (Kaph+Lesh, Kaph+Waw, Mem+Lesh, Pe+Lesh, Pe+Waw).
- **Dots below dead key.** `Shift+backtick` starts a new dead key that adds
  TWO DOTS BELOW (U+10F85) to any subsequent letter — complementing the existing
  dots-above dead key on plain backtick.
- **Expanded dead-key coverage.** The dots-above dead key now works with all 18
  Old Uyghur letters (previously only 3).

## New key mappings

| Key | Character | Purpose |
|-----|-----------|---------|
| `AltGr + =` | ZWJ (U+200D) | Force joining between letters |
| `AltGr + -` | ZWNJ (U+200C) | Break joining between letters |
| `AltGr + /` | Tatweel (U+0640) | Kashida elongation |
| `,` | DOT ABOVE (U+10F82) | Quick combining mark access |
| `.` | TWO DOTS ABOVE (U+10F84) | Quick combining mark access |
| `Shift + backtick` | Dead key → dots below | Add U+10F85 to next letter |
| `AltGr+/` → `K` → `L` | Kaph + Lesh | Ligature compose |
| `AltGr+/` → `K` → `O` | Kaph + Waw | Ligature compose |
| `AltGr+/` → `M` → `L` | Mem + Lesh | Ligature compose |
| `AltGr+/` → `P` → `L` | Pe + Lesh | Ligature compose |
| `AltGr+/` → `P` → `O` | Pe + Waw | Ligature compose |

*(Phonetic layout keys shown; QWERTY layout uses positional equivalents.)*

## Font compatibility

All three tested fonts have identical Old Uyghur codepoint coverage (26/26):

| Font | Total glyphs | Old Uyghur codepoints | OpenType features |
|------|-------------|----------------------|-------------------|
| QochoOldUyghur-Regular v1.000 | 109 | 26 | aalt, ccmp, dlig, fina, init, medi, rclt, rlig, salt |
| NotoSerifOldUyghur (fixed) | 352 | 26 | Full shaping |
| Ugurica Regular | 352 | 26 | Full shaping |

## Recommended applications

| Application | Old Uyghur shaping | Notes |
|-------------|-------------------|-------|
| **LibreOffice Writer** | ✅ Excellent | Full HarfBuzz shaping — best choice |
| **Firefox / Chrome / Edge** | ✅ Good | Web browsers with HarfBuzz |
| **Microsoft Word** | ⚠️ Partial | May not fully support SMP script shaping |
| **Notepad** | ❌ Poor | No complex text layout |

## Upgrading from 1.0.0

Drop-in upgrade. Copy the new `layouts/old_uyghur.json` and
`layouts/old_uyghur_qwerty.json` into your layouts directory. The app
hot-reloads changed files automatically — no restart needed.

Custom layouts are unaffected; only the two bundled layouts changed.

## Full changelog

See [`../CHANGELOG.md`](../CHANGELOG.md).
