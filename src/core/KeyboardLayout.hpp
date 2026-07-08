// KeyboardLayout.hpp — in-memory model of one keyboard layout.
//
// A KeyboardLayout is pure data + lookups (no OS calls, no I/O). It is produced
// by LayoutParser from a JSON file and consumed by Composer. This separation is
// what lets the whole mapping system be unit-tested without Windows.
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace core {

// Modifier "level" selecting which glyph a physical key produces.
enum class Level : uint8_t {
    Base        = 0,
    Shift       = 1,
    AltGr       = 2,   // right Alt / Ctrl+Alt
    ShiftAltGr  = 3,
    Count       = 4
};

// How Caps Lock affects letter keys.
enum class CapsMode : uint8_t {
    Ignore,     // Caps Lock has no effect (default for non-cased scripts)
    ShiftLetters, // Caps acts like Shift, but only for keys flagged cased
    Invert      // Caps inverts the Shift state for cased keys
};

// What a key does at a given level.
enum class ActionKind : uint8_t {
    None,       // unmapped -> pass through to the OS
    Emit,       // output a fixed string of codepoints
    DeadKey,    // begin/participate in a dead-key composition
    Compose     // begin a multi-key compose sequence (e.g. Compose a e -> æ)
};

struct KeyAction {
    ActionKind            kind = ActionKind::None;
    std::u32string        output;      // for Emit: the codepoints to produce
    std::string           deadKey;     // for DeadKey: the dead-key class id
    bool                  cased = false; // subject to Caps Lock (letters)
};

// One physical key: an action per modifier level.
struct KeyDef {
    KeyAction levels[static_cast<size_t>(Level::Count)];
    bool cased = false; // convenience OR of level cased flags
};

// A dead-key composition table: (deadKeyId, nextCodepoint) -> result string.
// Also carries the fallback output emitted when the dead key is followed by an
// unmapped key (typically the standalone diacritic glyph).
struct DeadKey {
    std::string    id;
    std::u32string standalone;                         // emitted alone / on no-match
    std::map<char32_t, std::u32string> compositions;   // next cp -> result
};

// A ligature: an ordered input codepoint sequence that collapses to a result.
struct Ligature {
    std::u32string sequence; // recently-emitted codepoints to match (suffix)
    std::u32string result;   // replacement
};

// A compose sequence: after a Compose action, the codepoints produced by the
// following keys are matched against `keys`; a full match emits `output`.
struct ComposeSequence {
    std::u32string keys;     // ordered produced-codepoints to match
    std::u32string output;   // result on exact match
};

// Result of probing the compose table with a partial buffer.
enum class ComposeMatch { None, Prefix, Exact };

struct LayoutMeta {
    std::string id          = "";        // stable identifier (usually file stem)
    std::string name        = "unnamed";
    std::string language    = "";
    std::string description = "";
    std::string author      = "";
    int         version     = 1;
};

class KeyboardLayout {
public:
    // --- Metadata / behavior flags ---
    const LayoutMeta& meta() const { return m_meta; }
    LayoutMeta&       meta()       { return m_meta; }

    CapsMode capsMode() const { return m_capsMode; }
    void     setCapsMode(CapsMode m) { m_capsMode = m; }

    bool normalize() const { return m_normalize; }
    void setNormalize(bool n) { m_normalize = n; }

    // --- Keys ---
    void setKey(unsigned vk, const KeyDef& def) { m_keys[vk] = def; }
    const KeyDef* key(unsigned vk) const {
        auto it = m_keys.find(vk);
        return it == m_keys.end() ? nullptr : &it->second;
    }
    const std::unordered_map<unsigned, KeyDef>& keys() const { return m_keys; }

    // --- Dead keys ---
    void addDeadKey(const DeadKey& dk) { m_deadKeys[dk.id] = dk; }
    const DeadKey* deadKey(const std::string& id) const {
        auto it = m_deadKeys.find(id);
        return it == m_deadKeys.end() ? nullptr : &it->second;
    }
    const std::map<std::string, DeadKey>& deadKeys() const { return m_deadKeys; }

    // --- Ligatures (kept sorted longest-first for greedy matching) ---
    void addLigature(const Ligature& lig) { m_ligatures.push_back(lig); sortLigatures(); }
    const std::vector<Ligature>& ligatures() const { return m_ligatures; }

    // --- Compose sequences ---
    // addCompose builds hash indexes so composeLookup is O(buffer length),
    // independent of the table size (fast even with thousands of sequences).
    void addCompose(const ComposeSequence& c);
    const std::vector<ComposeSequence>& composeSequences() const { return m_compose; }

    // Probe the compose table with a partial buffer. On Exact, `out` is set.
    ComposeMatch composeLookup(const std::u32string& buffer, std::u32string& out) const;

    // Resolve the action for (vk, level), honoring Caps Lock.
    // capsOn/shiftDown are the live modifier states.
    const KeyAction* resolve(unsigned vk, bool shiftDown, bool altGrDown, bool capsOn) const;

    // Longest ligature suffix that matches the tail of `recent`. nullopt if none.
    std::optional<Ligature> matchLigature(const std::u32string& recent) const;

private:
    void sortLigatures();

    LayoutMeta                             m_meta;
    CapsMode                               m_capsMode  = CapsMode::Ignore;
    bool                                   m_normalize = true;
    std::unordered_map<unsigned, KeyDef>   m_keys;
    std::map<std::string, DeadKey>         m_deadKeys;
    std::vector<Ligature>                  m_ligatures;
    std::vector<ComposeSequence>           m_compose;

    // Indexes for O(1) compose matching (built by addCompose).
    std::unordered_map<std::u32string, std::u32string> m_composeExact;   // full seq -> output
    std::unordered_set<std::u32string>                 m_composePrefix;  // all proper prefixes
    size_t                                             m_composeMaxLen = 0;
};

} // namespace core
