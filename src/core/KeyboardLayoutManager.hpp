// KeyboardLayoutManager.hpp — active-layout ownership and runtime switching.
//
// Holds a LayoutRegistry plus the currently active layout. The active layout is
// stored behind a stable pointer (std::unique_ptr), so consumers (the keyboard
// engine) can hold KeyboardLayout* across switches as long as they refresh it
// from the change callback. Switching/reloading parses + validates fresh.
#pragma once

#include "LayoutRegistry.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace core {

class KeyboardLayoutManager {
public:
    // Callback invoked whenever the active layout pointer changes (including
    // reloads). The pointer is valid until the next change; may be nullptr.
    using ChangeCallback = std::function<void(const KeyboardLayout*)>;

    // Scan `directory`, then activate `preferredId` (or the first valid layout
    // if the preferred id is missing/invalid). Returns true if a layout became
    // active. `err` (optional) receives a human-readable reason on failure.
    bool initialize(const std::string& directory,
                    const std::string& preferredId,
                    std::string* err = nullptr);

    // Re-scan the directory (e.g. after files are added/removed).
    size_t rescan(const std::string& directory);

    const std::vector<LayoutInfo>& available() const { return m_registry.layouts(); }

    const KeyboardLayout* current() const { return m_current.get(); }
    const std::string&    currentId() const { return m_currentId; }

    // Activate another layout by id. Returns true on success; on failure the
    // previous layout stays active and diagnostics land in last*().
    bool switchTo(const std::string& id);

    // Reparse + reactivate the current layout (for hot reload).
    bool reloadCurrent();

    void setOnChange(ChangeCallback cb) { m_onChange = std::move(cb); }

    const std::vector<std::string>& lastErrors()   const { return m_lastErrors; }
    const std::vector<std::string>& lastWarnings() const { return m_lastWarnings; }

private:
    bool activate(const std::string& id);

    LayoutRegistry                  m_registry;
    std::unique_ptr<KeyboardLayout> m_current;
    std::string                     m_currentId;
    ChangeCallback                  m_onChange;
    std::vector<std::string>        m_lastErrors;
    std::vector<std::string>        m_lastWarnings;
};

} // namespace core
