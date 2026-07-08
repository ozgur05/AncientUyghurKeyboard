#include "KeyboardLayout.hpp"
#include "VirtualKeys.hpp"

#include <algorithm>

namespace core {

const KeyAction* KeyboardLayout::resolve(unsigned vk, bool shiftDown,
                                         bool altGrDown, bool capsOn) const
{
    const KeyDef* def = key(vk);
    if (!def) return nullptr;

    // Determine effective Shift after applying Caps Lock policy.
    bool effectiveShift = shiftDown;
    const bool isCased = def->cased && vk::isLetter(vk);

    if (isCased && capsOn) {
        switch (m_capsMode) {
            case CapsMode::Ignore:                              break;
            case CapsMode::ShiftLetters: effectiveShift = true; break;
            case CapsMode::Invert:       effectiveShift = !shiftDown; break;
        }
    }

    // Pick the level. AltGr takes precedence over Shift selection axis.
    Level lvl;
    if (altGrDown)      lvl = effectiveShift ? Level::ShiftAltGr : Level::AltGr;
    else                lvl = effectiveShift ? Level::Shift      : Level::Base;

    const KeyAction* action = &def->levels[static_cast<size_t>(lvl)];

    // Graceful fallback for undefined levels:
    //   Shift+AltGr -> AltGr -> Base
    //   AltGr       -> Base
    //   Shift       -> Base  ONLY for cased (letter) keys, so that Shift+letter
    //                 still types the letter in this caseless script, while
    //                 Shift+digit / Shift+punct fall through to the OS symbol.
    if (action->kind == ActionKind::None) {
        if (lvl == Level::ShiftAltGr) {
            action = &def->levels[static_cast<size_t>(Level::AltGr)];
            if (action->kind == ActionKind::None)
                action = &def->levels[static_cast<size_t>(Level::Base)];
        } else if (lvl == Level::AltGr) {
            action = &def->levels[static_cast<size_t>(Level::Base)];
        } else if (lvl == Level::Shift && isCased) {
            action = &def->levels[static_cast<size_t>(Level::Base)];
        }
    }

    return action->kind == ActionKind::None ? nullptr : action;
}

std::optional<Ligature> KeyboardLayout::matchLigature(const std::u32string& recent) const
{
    // m_ligatures is sorted longest-first; return the first suffix match.
    for (const auto& lig : m_ligatures) {
        if (lig.sequence.empty()) continue;
        if (recent.size() < lig.sequence.size()) continue;
        if (recent.compare(recent.size() - lig.sequence.size(),
                           lig.sequence.size(), lig.sequence) == 0)
            return lig;
    }
    return std::nullopt;
}

void KeyboardLayout::addCompose(const ComposeSequence& c)
{
    m_compose.push_back(c);
    if (c.keys.empty())
        return;
    m_composeExact[c.keys] = c.output;              // last definition wins
    for (size_t i = 1; i < c.keys.size(); ++i)      // every proper prefix
        m_composePrefix.insert(c.keys.substr(0, i));
    if (c.keys.size() > m_composeMaxLen)
        m_composeMaxLen = c.keys.size();
}

ComposeMatch KeyboardLayout::composeLookup(const std::u32string& buffer,
                                           std::u32string& out) const
{
    if (buffer.empty())
        return m_compose.empty() ? ComposeMatch::None : ComposeMatch::Prefix;

    // Early-out: nothing can match a buffer longer than the longest sequence.
    if (buffer.size() > m_composeMaxLen)
        return ComposeMatch::None;

    // Exact match wins (greedy shortest), then proper-prefix. Both are O(1)
    // hash probes, so lookup cost is independent of the table size.
    auto it = m_composeExact.find(buffer);
    if (it != m_composeExact.end()) {
        out = it->second;
        return ComposeMatch::Exact;
    }
    if (m_composePrefix.find(buffer) != m_composePrefix.end())
        return ComposeMatch::Prefix;
    return ComposeMatch::None;
}

void KeyboardLayout::sortLigatures()
{
    std::sort(m_ligatures.begin(), m_ligatures.end(),
              [](const Ligature& a, const Ligature& b) {
                  return a.sequence.size() > b.sequence.size();
              });
}

} // namespace core
