#include "LayoutRegistry.hpp"
#include "LayoutParser.hpp"
#include "LayoutValidator.hpp"

#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace core {

size_t LayoutRegistry::scan(const std::string& directory)
{
    m_layouts.clear();

    std::error_code ec;
    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec))
        return 0;

    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        const fs::path& p = entry.path();
        if (p.extension() != ".json") continue;

        LayoutInfo info;
        info.path = p.string();
        info.id   = p.stem().string();
        info.name = info.id;

        // Parse just enough to get name/id and validity for the listing.
        ParseResult pr = LayoutParser::parseFile(info.path);
        if (pr.ok()) {
            const auto& meta = pr.layout->meta();
            if (!meta.id.empty())   info.id   = meta.id;
            if (!meta.name.empty()) info.name = meta.name;
            ValidationReport vr = LayoutValidator::validate(*pr.layout);
            info.valid = vr.ok();
        } else {
            info.valid = false;
        }
        m_layouts.push_back(std::move(info));
    }

    std::sort(m_layouts.begin(), m_layouts.end(),
              [](const LayoutInfo& a, const LayoutInfo& b) { return a.name < b.name; });
    return m_layouts.size();
}

const LayoutInfo* LayoutRegistry::find(const std::string& id) const
{
    for (const auto& l : m_layouts)
        if (l.id == id) return &l;
    return nullptr;
}

LayoutRegistry::LoadOutcome LayoutRegistry::load(const std::string& id) const
{
    LoadOutcome out;
    const LayoutInfo* info = find(id);
    if (!info) {
        out.errors.push_back("no layout with id '" + id + "'");
        return out;
    }

    ParseResult pr = LayoutParser::parseFile(info->path);
    out.errors   = pr.errors;
    out.warnings = pr.warnings;
    if (!pr.ok())
        return out;

    ValidationReport vr = LayoutValidator::validate(*pr.layout);
    for (auto& w : vr.warnings) out.warnings.push_back(w);
    for (auto& e : vr.errors)   out.errors.push_back(e);
    if (!vr.ok())
        return out; // leave layout unset: validation errors are fatal

    out.layout = std::move(*pr.layout);
    return out;
}

} // namespace core
