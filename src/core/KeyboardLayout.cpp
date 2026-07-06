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

ComposeMatch KeyboardLayout::composeLookup(const std::u32string& buffer,
                                           std::u32string& out) const
{
    if (buffer.empty())
        return m_compose.empty() ? ComposeMatch::None : ComposeMatch::Prefix;

    bool prefix = false;
    for (const auto& seq : m_compose) {
        if (seq.keys == buffer) {           // exact (greedy: first exact wins)
            out = seq.output;
            return ComposeMatch::Exact;
        }
        if (seq.keys.size() > buffer.size() &&
            seq.keys.compare(0, buffer.size(), buffer) == 0)
            prefix = true;                  // buffer is a proper prefix of seq
    }
    return prefix ? ComposeMatch::Prefix : ComposeMatch::None;
}

void KeyboardLayout::sortLigatures()
{
    std::sort(m_ligatures.begin(), m_ligatures.end(),
              [](const Ligature& a, const Ligature& b) {
                  return a.sequence.size() > b.sequence.size();
              });
}

} // namespace core
