# Release Notes — v1.1.1

**Ancient Uyghur Keyboard 1.1.1** — Bug-fix release for full
QochoOldUyghur-Regular v1.000 layout parity.

## What's fixed

### Duplicate `OEM_2` key in QWERTY layout
The v1.1.0 `old_uyghur_qwerty.json` contained two `"OEM_2"` entries — one
mapping `AltGr+/` to Tatweel (`U+0640`) and another mapping it to the compose
trigger. JSON parsing silently kept only the last definition, so **Tatweel was
unreachable**. This release merges them into a single entry. Tatweel is now
accessible via `AltGr+\` (direct) and `AltGr+/` → `J` (compose).

### Compose prefix collision
The Manichaean Double Dot (`U+10AF2`) compose was a single-key `M` (`U+10F79`),
which collided with the `M+L` (Mem+Lesh) ligature compose prefix. Changed to a
two-key sequence `M M` to eliminate ambiguity.

### Misleading comments
Comments in `old_uyghur.json` incorrectly stated `AltGr+M` inserted Manichaean
punctuation and `AltGr+/` inserted Tatweel directly. Corrected to reflect actual
behavior: `AltGr+/` is the compose trigger; Tatweel/Manichaean are via compose
or direct `AltGr` on `\`/`;`/`T`/`M` keys.

## What's new

### Dotted Circle (`U+25CC`)
Added to both layouts on `AltGr+[` and via compose (`AltGr+/` → `;`). Essential
for displaying standalone combining diacritics (◌𐾂 ◌𐾃 ◌𐾄 ◌𐾅).

### Direct AltGr mappings
Both layouts now have direct `AltGr` access to all shaping controls:

| Key | Phonetic layout | QWERTY layout |
|-----|----------------|---------------|
| `AltGr + -` | ZWNJ (U+200C) | ZWNJ (U+200C) |
| `AltGr + =` | ZWJ (U+200D) | ZWJ (U+200D) |
| `AltGr + [` | Dotted Circle (U+25CC) | Dotted Circle (U+25CC) |
| `AltGr + \` | Tatweel (U+0640) | Tatweel (U+0640) |
| `AltGr + ;` | Manichaean (U+10AF2) | Manichaean (U+10AF2) |
| `AltGr + T` | Tatweel (U+0640) | — |
| `AltGr + M` | Manichaean (U+10AF2) | — |
| `AltGr + J` | ZWJ (U+200D) | — |
| `AltGr + N` | ZWNJ (U+200C) | — |

### Full dead-key coverage
QWERTY layout's `dots_above` and `dots_below` dead keys now cover all 18 Old
Uyghur letters (was only 6 in v1.1.0).

## Validation

Both layouts pass automated validation:
- Zero duplicate JSON keys
- All 31 required codepoints reachable (26 Old Uyghur + U+10AF2, U+0640, U+200C, U+200D, U+25CC)
- All 5 ligature compose sequences present

## Upgrading from 1.1.0

Drop-in replacement. Copy `layouts/old_uyghur.json` and
`layouts/old_uyghur_qwerty.json` to your layouts directory. The app hot-reloads
automatically.

## Full changelog

See [`../CHANGELOG.md`](../CHANGELOG.md).
