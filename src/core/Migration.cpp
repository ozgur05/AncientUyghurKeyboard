#include "Migration.hpp"

#include <algorithm>
#include <unordered_set>

namespace core {

ConfigMap Migration::defaults()
{
    return {
        { "enabled",   "1" },
        { "layout",    "old_uyghur" },
        { "log_level", "info" },
    };
}

ConfigMap Migration::migrateConfig(const ConfigMap& existing,
                                   const Version& /*from*/, const Version& /*to*/)
{
    ConfigMap out = existing;

    // 1. Rename deprecated keys (preserve the value; don't clobber a key that
    //    already exists under the new name).
    static const std::pair<const char*, const char*> kRenames[] = {
        { "enable", "enabled" },   // pre-0.4 spelling
    };
    for (const auto& [oldKey, newKey] : kRenames) {
        auto it = out.find(oldKey);
        if (it != out.end()) {
            if (out.find(newKey) == out.end())
                out[newKey] = it->second;
            out.erase(it);
        }
    }

    // 2. Fill any missing required keys with defaults (never overwrite a value
    //    the user already set — that is the preservation guarantee).
    for (const auto& [k, v] : defaults())
        out.emplace(k, v);

    return out;
}

std::vector<std::string> Migration::planLayoutSeed(
    const std::vector<std::string>& bundled,
    const std::vector<std::string>& existing)
{
    std::unordered_set<std::string> have(existing.begin(), existing.end());
    std::vector<std::string> toCopy;
    for (const auto& id : bundled)
        if (have.find(id) == have.end())
            toCopy.push_back(id);
    return toCopy;
}

} // namespace core
