# API Reference

Reference for the public classes of the portable core (`namespace core`, in
`src/core/`). All types are declared in the header of the same name. The core is
Win32-free and has **no global mutable state**; unless noted, a class instance is
**not** internally synchronized — do not share one instance across threads
without external locking. Distinct instances are independent and safe to use
concurrently.

---

## `core::Version` — `Version.hpp`

Semantic-ish `major.minor.patch` version with parsing and comparison.

| Member | Signature | Returns / notes |
|---|---|---|
| `parse` | `static std::optional<Version> parse(const std::string&)` | Parses `"1"`, `"1.2"`, `"1.2.3"`, optional leading `v`, ignores `-`/`+` suffix. `nullopt` on malformed input. |
| `toString` | `std::string toString() const` | `"major.minor.patch"`. |
| `compare` / operators | `int compare(const Version&) const`; `== != < > <= >=` | Ordered by (major, minor, patch). |

```cpp
auto v = core::Version::parse("v1.2.3-rc1").value(); // 1.2.3
assert(core::Version(1,2,0) < core::Version(1,10,0)); // numeric, not lexical
```
Thread safety: value type; freely copyable; immutable after construction.

---

## `core::KeyboardLayout` — `KeyboardLayout.hpp`

The parsed in-memory model of one layout. Produced by `LayoutParser`, consumed by
`Composer`, `LayoutValidator`, and `LayoutSerializer`.

Key methods:

- `const LayoutMeta& meta() const` / `LayoutMeta& meta()` — id, name, language,
  description, author, version.
- `CapsMode capsMode() const` / `void setCapsMode(CapsMode)` — `Ignore`,
  `ShiftLetters`, or `Invert`.
- `bool normalize() const` / `void setNormalize(bool)` — apply NFC on emit.
- `void setKey(unsigned vk, const KeyDef&)` / `void removeKey(unsigned)` /
  `const KeyDef* key(unsigned) const` — per-key access (`nullptr` if unmapped).
- `void addDeadKey(const DeadKey&)` / `removeDeadKey(id)` / `deadKey(id)`.
- `void addLigature(const Ligature&)` / `clearLigatures()` / `ligatures()`.
- `void addCompose(const ComposeSequence&)` / `clearCompose()` /
  `composeSequences()` — `addCompose` builds hash indexes so `composeLookup` is
  **O(buffer length)**, independent of table size.
- `const KeyAction* resolve(unsigned vk, bool shift, bool altGr, bool caps) const`
  — the effective action for a key, honoring Caps-Lock policy and level fallback
  (Shift→Base for cased letters; AltGr→Base). `nullptr` if unmapped.
- `ComposeMatch composeLookup(const std::u32string& buffer, std::u32string& out) const`
  — `None` / `Prefix` / `Exact` (sets `out` on `Exact`).

Thread safety: mutable model; not synchronized. Copyable (used for undo
snapshots). Read-only concurrent access to a `const` instance is safe.

---

## `core::LayoutParser` — `LayoutParser.hpp`

JSON → `KeyboardLayout` (structural parse + scalar validation).

- `static ParseResult parseString(const std::string& json)`
- `static ParseResult parseFile(const std::string& path)`

`ParseResult { std::optional<KeyboardLayout> layout; std::vector<std::string>
errors, warnings; bool ok() const; }`. `ok()` is true only when a layout was
produced with no errors. Never throws.

```cpp
auto r = core::LayoutParser::parseString(json);
if (r.ok()) use(*r.layout);
else for (auto& e : r.errors) log(e);
```

---

## `core::LayoutValidator` — `LayoutValidator.hpp`

Semantic validation of a built layout.

- `static ValidationReport validate(const KeyboardLayout&)`

`ValidationReport { std::vector<std::string> errors, warnings; bool ok() const; }`.
Errors (layout should be rejected): no keys, dangling dead-key reference,
non-scalar codepoint. Warnings (advisory): duplicate glyphs, empty dead keys,
duplicate/unreachable compose sequences.

---

## `core::LayoutSerializer` — `LayoutSerializer.hpp`

`KeyboardLayout` → pretty JSON, round-trip stable with `LayoutParser`.

- `static std::string toJson(const KeyboardLayout&)` — codepoints written as
  `"U+XXXX"` tokens; keys sorted by VK for deterministic output.

---

## `core::LayoutRegistry` — `LayoutRegistry.hpp`

Discovery of layout files in a directory (`std::filesystem`).

- `size_t scan(const std::string& dir)` — populate the list; returns count.
  Exception-safe.
- `const std::vector<LayoutInfo>& layouts() const` — `{id, name, path, valid}`.
- `const LayoutInfo* find(const std::string& id) const`.
- `LoadOutcome load(const std::string& id) const` — parse + validate a layout.

---

## `core::KeyboardLayoutManager` — `KeyboardLayoutManager.hpp`

Owns the active layout and switches it at runtime.

- `bool initialize(const std::string& dir, const std::string& preferredId, std::string* err = nullptr)`
- `bool switchTo(const std::string& id)` / `bool reloadCurrent()`
- `const KeyboardLayout* current() const` — stable pointer for the active layout.
- `void setOnChange(std::function<void(const KeyboardLayout*)>)` — fired whenever
  the active pointer changes (consumers re-point their `Composer`).
- `const std::vector<LayoutInfo>& available() const`; `lastErrors()`,
  `lastWarnings()`.

Thread safety: designed to be driven from one thread (the UI/message thread).

---

## `core::Composer` — `Composer.hpp`

The input state machine shared by every backend.

- `void setLayout(const KeyboardLayout*)` — point at a layout (resets state).
- `EmitOp process(const KeyInput& in)` — translate one key-down into an edit.
- `void reset()` — clear dead-key/compose/window state (on focus change).
- `bool deadPending() const`, `bool composing() const`, `const std::u32string& window() const`.

`KeyInput { unsigned vk; bool shift, altgr, caps; }` →
`EmitOp { bool suppress; int backspaces; std::u32string insert; }`.

```cpp
core::Composer c; c.setLayout(mgr.current());
core::KeyInput k{ core::vk::KeyA, false, false, false };
core::EmitOp op = c.process(k); // e.g. {suppress:true, backspaces:0, insert:U"\U00010F70"}
```
Thread safety: stateful; one `Composer` per input thread.

---

## `core::LayoutDocument` — `LayoutDocument.hpp`

Editor document model used by the designer: a layout plus undo/redo, dirty flag,
and clipboard.

- `bool loadFromJson(const std::string&, std::string* err = nullptr)`; `std::string toJson() const`; `void newLayout(id, name)`.
- `void setKeyAction(unsigned vk, Level, const KeyAction&)`; `void clearKey(vk)`.
- `bool copyKey(vk)` / `bool pasteKey(vk)` / `bool hasClipboard() const`.
- `void setMeta(...)`, `setCapsMode`, `setNormalize`, `setDeadKey`, `removeDeadKey`,
  `setComposeSequences`, `setLigatures`.
- `bool undo()` / `bool redo()` / `canUndo()` / `canRedo()`.
- `ValidationReport validate() const`; `bool dirty() const`; `void markSaved()`.

Every mutation snapshots the layout for undo and marks the document dirty.

---

## Supporting utilities

- **`core::unicode`** (`Unicode.hpp`) — `toUtf16`, `utf8ToUtf32`, `utf32ToUtf8`,
  `utf16ToUtf32`, `utf32ToUtf16`, `isValidScalar`. Total functions: invalid input
  → `U+FFFD`. Header-only, stateless, thread-safe.
- **`core::vk`** (`VirtualKeys.hpp`) — `fromName`, `toName`, `isLetter`, VK
  constants. Stateless.
- **`core::normalizer`** (`Normalizer.hpp`) — `combiningClass`, `canonicalOrder`,
  `compose`, `toNFC` (curated tables). Stateless.
- **`core::UnicodeNames`** (`UnicodeNames.hpp`) — `name(cp)`, `byCodepoint(cp)`,
  `search(query, limit)`. Curated table; stateless.
- **`core::KeyCaps`** (`KeyCaps.hpp`) — `caps(BoardType)`, `width`, `height`.
  ANSI/ISO geometry. Stateless.
- **`core::Stopwatch`** (`Stopwatch.hpp`) — `millis`, `micros`, `nanos`,
  `reset`. Wraps `steady_clock`.

For build provenance, `buildinfo::full()` / `version()` / `gitHash()` /
`buildNumber()` / `timestamp()` (`src/BuildInfo.hpp`) return the embedded build
metadata.
