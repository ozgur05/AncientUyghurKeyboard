// LayoutRegistry.hpp — discovery of available layouts in a directory.
//
// Scans a folder for *.json layout files, parsing each one's metadata so the
// UI can present a list, and loads a full validated layout on demand by id.
// Uses std::filesystem (portable, no external dependency).
#pragma once

#include "KeyboardLayout.hpp"
#include <string>
#include <vector>
#include <optional>

namespace core {

struct LayoutInfo {
    std::string id;      // stable identifier (file stem, or meta.id if present)
    std::string name;    // display name (meta.name)
    std::string path;    // absolute/normalized path to the .json file
    bool        valid = true; // parsed + validated cleanly during scan
};

class LayoutRegistry {
public:
    struct LoadOutcome {
        std::optional<KeyboardLayout> layout;
        std::vector<std::string>      errors;
        std::vector<std::string>      warnings;
        bool ok() const { return layout.has_value() && errors.empty(); }
    };

    // (Re)scan a directory. Returns the number of layout files discovered.
    size_t scan(const std::string& directory);

    const std::vector<LayoutInfo>& layouts() const { return m_layouts; }
    const LayoutInfo* find(const std::string& id) const;

    // Parse + validate the layout with the given id.
    LoadOutcome load(const std::string& id) const;

private:
    std::vector<LayoutInfo> m_layouts;
};

} // namespace core
