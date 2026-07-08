#include "LayoutDocument.hpp"
#include "LayoutParser.hpp"
#include "LayoutSerializer.hpp"
#include "VirtualKeys.hpp"

namespace core {

LayoutDocument::LayoutDocument()
{
    newLayout("new_layout", "New Layout");
    m_dirty = false;
    m_undo.clear();
    m_redo.clear();
}

void LayoutDocument::newLayout(const std::string& id, const std::string& name)
{
    KeyboardLayout fresh;
    LayoutMeta meta;
    meta.id      = id;
    meta.name    = name;
    meta.version = 1;
    fresh.meta() = meta;
    fresh.setCapsMode(CapsMode::Ignore);
    fresh.setNormalize(true);
    m_layout = fresh;
    m_undo.clear();
    m_redo.clear();
    m_clip.reset();
    m_dirty = true; // a new unsaved layout is dirty
}

bool LayoutDocument::loadFromJson(const std::string& json, std::string* error)
{
    ParseResult r = LayoutParser::parseString(json);
    if (!r.ok()) {
        if (error) {
            std::string msg;
            for (const auto& e : r.errors) { if (!msg.empty()) msg += "; "; msg += e; }
            *error = msg;
        }
        return false;
    }
    m_layout = std::move(*r.layout);
    m_undo.clear();
    m_redo.clear();
    m_clip.reset();
    m_dirty = false;
    return true;
}

std::string LayoutDocument::toJson() const
{
    return LayoutSerializer::toJson(m_layout);
}

void LayoutDocument::pushUndo()
{
    m_undo.push_back(m_layout);              // snapshot current state
    if (m_undo.size() > kMaxHistory)
        m_undo.erase(m_undo.begin());
    m_redo.clear();                          // a new edit invalidates redo
    m_dirty = true;
}

bool LayoutDocument::undo()
{
    if (m_undo.empty()) return false;
    m_redo.push_back(m_layout);
    m_layout = std::move(m_undo.back());
    m_undo.pop_back();
    m_dirty = true;
    return true;
}

bool LayoutDocument::redo()
{
    if (m_redo.empty()) return false;
    m_undo.push_back(m_layout);
    m_layout = std::move(m_redo.back());
    m_redo.pop_back();
    m_dirty = true;
    return true;
}

void LayoutDocument::setKeyAction(unsigned vk, Level level, const KeyAction& action)
{
    pushUndo();
    KeyDef def;
    if (const KeyDef* existing = m_layout.key(vk))
        def = *existing;
    def.levels[static_cast<size_t>(level)] = action;
    // Recompute the cased convenience flag: letters are Caps-Lock sensitive.
    def.cased = vk::isLetter(vk);
    for (auto& lvl : def.levels)
        if (def.cased && lvl.kind == ActionKind::Emit) lvl.cased = true;

    // If every level is now None, drop the key entirely.
    bool anyMapped = false;
    for (const auto& lvl : def.levels)
        if (lvl.kind != ActionKind::None) { anyMapped = true; break; }
    if (anyMapped) m_layout.setKey(vk, def);
    else           m_layout.removeKey(vk);
}

void LayoutDocument::clearKey(unsigned vk)
{
    pushUndo();
    m_layout.removeKey(vk);
}

bool LayoutDocument::copyKey(unsigned vk)
{
    const KeyDef* def = m_layout.key(vk);
    if (!def) { m_clip.reset(); return false; }
    m_clip = *def;
    return true;
}

bool LayoutDocument::pasteKey(unsigned vk)
{
    if (!m_clip) return false;
    pushUndo();
    m_layout.setKey(vk, *m_clip);
    return true;
}

void LayoutDocument::setMeta(const LayoutMeta& meta)
{
    pushUndo();
    m_layout.meta() = meta;
}

void LayoutDocument::setCapsMode(CapsMode m)
{
    pushUndo();
    m_layout.setCapsMode(m);
}

void LayoutDocument::setNormalize(bool n)
{
    pushUndo();
    m_layout.setNormalize(n);
}

void LayoutDocument::setDeadKey(const DeadKey& dk)
{
    pushUndo();
    m_layout.addDeadKey(dk); // add or replace by id
}

void LayoutDocument::removeDeadKey(const std::string& id)
{
    pushUndo();
    m_layout.removeDeadKey(id);
}

void LayoutDocument::setComposeSequences(const std::vector<ComposeSequence>& seqs)
{
    pushUndo();
    m_layout.clearCompose();
    for (const auto& s : seqs) m_layout.addCompose(s);
}

void LayoutDocument::setLigatures(const std::vector<Ligature>& ligs)
{
    pushUndo();
    m_layout.clearLigatures();
    for (const auto& l : ligs) m_layout.addLigature(l);
}

} // namespace core
