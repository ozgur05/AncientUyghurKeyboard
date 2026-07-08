// LayoutDocument.hpp — editable layout document with undo/redo.
//
// Wraps a KeyboardLayout with everything the designer needs that is NOT UI:
// mutations (edit a key level, metadata, dead keys, compose sequences,
// ligatures), a full undo/redo history, a dirty flag, clipboard copy/paste of a
// key's mappings, load/import (via LayoutParser) and serialize/export (via
// LayoutSerializer), and on-demand validation (via LayoutValidator).
//
// The undo model is simple and robust: each mutation snapshots the whole layout
// onto an undo stack before applying. Layouts are small, so this is cheap and
// impossible to get subtly wrong. Portable and unit-tested.
#pragma once

#include "KeyboardLayout.hpp"
#include "LayoutValidator.hpp"
#include <string>
#include <vector>
#include <optional>

namespace core {

class LayoutDocument {
public:
    LayoutDocument();

    // ---- Load / save ----
    // Replace the document with a parsed layout. Returns false (and fills
    // `error`) if the text does not parse. Clears history and dirty flag.
    bool loadFromJson(const std::string& json, std::string* error = nullptr);
    // Serialize the current layout to JSON.
    std::string toJson() const;
    // Start a brand-new empty layout with sensible defaults.
    void newLayout(const std::string& id, const std::string& name);

    // ---- Access ----
    const KeyboardLayout& layout() const { return m_layout; }
    bool  dirty() const { return m_dirty; }
    void  markSaved() { m_dirty = false; }

    // ---- Undo / redo ----
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    bool undo();
    bool redo();

    // ---- Key editing ----
    // Set (or clear, when action.kind==None) one modifier level of one key.
    void setKeyAction(unsigned vk, Level level, const KeyAction& action);
    // Remove a key entirely.
    void clearKey(unsigned vk);

    // Clipboard: copy a key's four levels, paste them onto another key.
    bool copyKey(unsigned vk);
    bool hasClipboard() const { return m_clip.has_value(); }
    bool pasteKey(unsigned vk); // returns false if clipboard empty

    // ---- Metadata / behavior ----
    void setMeta(const LayoutMeta& meta);
    void setCapsMode(CapsMode m);
    void setNormalize(bool n);

    // ---- Dead keys / compose / ligatures ----
    void setDeadKey(const DeadKey& dk);         // add or replace by id
    void removeDeadKey(const std::string& id);
    void setComposeSequences(const std::vector<ComposeSequence>& seqs); // replace all
    void setLigatures(const std::vector<Ligature>& ligs);               // replace all

    // ---- Validation ----
    ValidationReport validate() const { return LayoutValidator::validate(m_layout); }

private:
    void pushUndo();          // snapshot before a mutation

    KeyboardLayout               m_layout;
    bool                         m_dirty = false;
    std::vector<KeyboardLayout>  m_undo;
    std::vector<KeyboardLayout>  m_redo;
    std::optional<KeyDef>        m_clip;   // copied key mappings

    static constexpr size_t kMaxHistory = 100;
};

} // namespace core
