#include "Composer.hpp"
#include "VirtualKeys.hpp"
#include "Normalizer.hpp"

#include <algorithm>

namespace core {

namespace {
size_t commonPrefix(const std::u32string& a, const std::u32string& b)
{
    size_t n = std::min(a.size(), b.size());
    size_t i = 0;
    while (i < n && a[i] == b[i]) ++i;
    return i;
}

bool isModifierVk(unsigned vk)
{
    return vk == vk::Shift || vk == vk::Control || vk == vk::Menu ||
           vk == vk::Capital || vk == vk::LWin || vk == vk::RWin;
}
} // namespace

void Composer::reset()
{
    m_recent.clear();
    m_pendingDead.clear();
    m_composing = false;
    m_composeBuf.clear();
}

EmitOp Composer::handleCompose(const std::u32string& produced)
{
    EmitOp o;
    o.suppress = true;              // keys are swallowed while composing
    m_composeBuf += produced;

    std::u32string out;
    switch (m_layout->composeLookup(m_composeBuf, out)) {
        case ComposeMatch::Exact:
            m_composing = false;
            m_composeBuf.clear();
            return commit(out);     // emit the composed result
        case ComposeMatch::Prefix:
            return o;               // wait for more keys
        case ComposeMatch::None:
        default:
            m_composing = false;    // dead end: abort, produce nothing
            m_composeBuf.clear();
            return o;
    }
}

std::u32string Composer::applyLigatures(std::u32string text) const
{
    if (!m_layout) return text;
    // Greedy, longest-first; loop because a replacement can complete another.
    for (int guard = 0; guard < 8; ++guard) {
        auto lig = m_layout->matchLigature(text);
        if (!lig) break;
        text.erase(text.size() - lig->sequence.size());
        text += lig->result;
    }
    return text;
}

EmitOp Composer::commit(const std::u32string& produced)
{
    EmitOp op;
    op.suppress = true;

    const std::u32string base = m_recent;
    std::u32string processed = base + produced;

    if (m_layout && m_layout->normalize())
        processed = normalizer::toNFC(processed);

    processed = applyLigatures(processed);

    // Diff the tracked tail against the new desired tail.
    size_t p = commonPrefix(base, processed);
    op.backspaces = static_cast<int>(base.size() - p); // 1 Backspace per codepoint
    op.insert     = processed.substr(p);

    // Update the window (cap to the last kWindow codepoints).
    if (processed.size() > kWindow)
        processed.erase(0, processed.size() - kWindow);
    m_recent = std::move(processed);
    return op;
}

EmitOp Composer::process(const KeyInput& in)
{
    EmitOp none; // suppress=false, empty
    if (!m_layout) return none;

    // Pure modifier presses never compose.
    if (isModifierVk(in.vk)) return none;

    // ---- Compose mode ---------------------------------------------------
    // While a compose sequence is active, keys feed the compose buffer. Editing
    // keys manage/cancel it; a key that can't contribute aborts compose and is
    // then processed normally (fall through).
    if (m_composing) {
        if (in.vk == vk::Back) {
            if (!m_composeBuf.empty()) m_composeBuf.pop_back();
            else                       m_composing = false;
            EmitOp o; o.suppress = true; return o;
        }
        if (in.vk == vk::Escape) {
            m_composing = false; m_composeBuf.clear();
            EmitOp o; o.suppress = true; return o;
        }
        const KeyAction* ca = m_layout->resolve(in.vk, in.shift, in.altgr, in.caps);
        if (ca && ca->kind == ActionKind::Emit && !ca->output.empty())
            return handleCompose(ca->output);
        // Non-contributing key: abort compose and continue normal handling.
        m_composing = false;
        m_composeBuf.clear();
    }

    // ---- Special editing keys -------------------------------------------
    if (in.vk == vk::Back) {
        if (!m_pendingDead.empty()) { m_pendingDead.clear(); EmitOp o; o.suppress = true; return o; }
        if (!m_recent.empty()) m_recent.pop_back();
        return none; // let the app delete
    }
    if (in.vk == vk::Enter || in.vk == vk::Tab) {
        EmitOp o;
        if (!m_pendingDead.empty()) {
            const DeadKey* dk = m_layout->deadKey(m_pendingDead);
            if (dk && !dk->standalone.empty()) o.insert = dk->standalone;
            m_pendingDead.clear();
        }
        m_recent.clear();          // hard boundary
        o.suppress = false;        // Enter/Tab still passes through
        return o;
    }
    if (in.vk == vk::Space) {
        if (!m_pendingDead.empty()) {
            // Dead + Space -> the spacing (standalone) diacritic.
            const DeadKey* dk = m_layout->deadKey(m_pendingDead);
            std::u32string standalone = dk ? dk->standalone : std::u32string();
            m_pendingDead.clear();
            EmitOp o = commit(standalone);
            return o; // suppress the space, emit the diacritic
        }
        m_recent.clear();          // word boundary
        return none;               // space passes through
    }

    // ---- Layout lookup ---------------------------------------------------
    const KeyAction* action = m_layout->resolve(in.vk, in.shift, in.altgr, in.caps);

    if (!action) {
        // Unmapped key. If a dead key is pending, flush its standalone form.
        if (!m_pendingDead.empty()) {
            const DeadKey* dk = m_layout->deadKey(m_pendingDead);
            std::u32string standalone = dk ? dk->standalone : std::u32string();
            m_pendingDead.clear();
            EmitOp o = commit(standalone);
            o.suppress = false;    // also let the physical key through
            return o;
        }
        m_recent.clear();          // cursor may move; drop the window
        return none;
    }

    if (action->kind == ActionKind::Compose) {
        EmitOp o;
        // Flush any pending dead key, then begin a fresh compose sequence.
        if (!m_pendingDead.empty()) {
            const DeadKey* prev = m_layout->deadKey(m_pendingDead);
            if (prev && !prev->standalone.empty()) o = commit(prev->standalone);
            m_pendingDead.clear();
        }
        m_composing  = true;
        m_composeBuf.clear();
        o.suppress = true;         // swallow; wait for the sequence
        return o;
    }

    if (action->kind == ActionKind::DeadKey) {
        EmitOp o;
        // Two dead keys in a row: the first resolves to its standalone form.
        if (!m_pendingDead.empty()) {
            const DeadKey* prev = m_layout->deadKey(m_pendingDead);
            if (prev && !prev->standalone.empty()) o = commit(prev->standalone);
        }
        m_pendingDead = action->deadKey;
        o.suppress = true;         // swallow; wait for the next key
        return o;
    }

    // Emit action.
    std::u32string produced = action->output;
    if (!m_pendingDead.empty()) {
        const DeadKey* dk = m_layout->deadKey(m_pendingDead);
        m_pendingDead.clear();
        if (dk) {
            char32_t nextCp = produced.empty() ? 0 : produced[0];
            auto it = dk->compositions.find(nextCp);
            if (it != dk->compositions.end())
                produced = it->second;                 // composed form
            else
                produced = dk->standalone + produced;   // no match: diacritic + letter
        }
    }
    return commit(produced);
}

} // namespace core
