#include "KeyboardLayoutManager.hpp"

namespace core {

bool KeyboardLayoutManager::activate(const std::string& id)
{
    LayoutRegistry::LoadOutcome out = m_registry.load(id);
    m_lastErrors   = out.errors;
    m_lastWarnings = out.warnings;
    if (!out.ok())
        return false;

    m_current   = std::make_unique<KeyboardLayout>(std::move(*out.layout));
    m_currentId = id;
    if (m_onChange)
        m_onChange(m_current.get());
    return true;
}

bool KeyboardLayoutManager::initialize(const std::string& directory,
                                       const std::string& preferredId,
                                       std::string* err)
{
    m_registry.scan(directory);

    if (m_registry.layouts().empty()) {
        if (err) *err = "no layout files found in " + directory;
        return false;
    }

    // Prefer the requested id when it is present and valid.
    if (const LayoutInfo* pref = m_registry.find(preferredId); pref && pref->valid) {
        if (activate(preferredId))
            return true;
    }

    // Otherwise activate the first valid layout.
    for (const auto& info : m_registry.layouts()) {
        if (info.valid && activate(info.id))
            return true;
    }

    if (err) *err = "no valid layout could be activated";
    return false;
}

size_t KeyboardLayoutManager::rescan(const std::string& directory)
{
    return m_registry.scan(directory);
}

bool KeyboardLayoutManager::switchTo(const std::string& id)
{
    if (id == m_currentId && m_current)
        return true; // already active
    return activate(id);
}

bool KeyboardLayoutManager::reloadCurrent()
{
    if (m_currentId.empty())
        return false;
    return activate(m_currentId);
}

} // namespace core
