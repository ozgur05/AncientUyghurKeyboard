#include "Version.hpp"

#include <sstream>
#include <cctype>

namespace core {

std::optional<Version> Version::parse(const std::string& in)
{
    // Strip a leading 'v'/'V' and any pre-release/build suffix.
    std::string s = in;
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return std::nullopt;
    s = s.substr(start);
    if (!s.empty() && (s[0] == 'v' || s[0] == 'V')) s.erase(0, 1);
    size_t cut = s.find_first_of("-+");
    if (cut != std::string::npos) s = s.substr(0, cut);
    // Trim trailing whitespace.
    size_t end = s.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) return std::nullopt;
    s = s.substr(0, end + 1);
    if (s.empty()) return std::nullopt;

    int parts[3] = {0, 0, 0};
    int idx = 0;
    std::string cur;
    auto flush = [&](void) -> bool {
        if (cur.empty()) return false;
        for (char c : cur) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        if (idx < 3) {
            try { parts[idx] = std::stoi(cur); }
            catch (...) { return false; }
        }
        ++idx;
        cur.clear();
        return true;
    };

    for (char c : s) {
        if (c == '.') { if (!flush()) return std::nullopt; }
        else          { cur += c; }
    }
    if (!flush()) return std::nullopt;
    if (idx == 0 || idx > 3) return std::nullopt;

    return Version{ parts[0], parts[1], parts[2] };
}

std::string Version::toString() const
{
    std::ostringstream o;
    o << major << '.' << minor << '.' << patch;
    return o.str();
}

int Version::compare(const Version& o) const
{
    if (major != o.major) return major < o.major ? -1 : 1;
    if (minor != o.minor) return minor < o.minor ? -1 : 1;
    if (patch != o.patch) return patch < o.patch ? -1 : 1;
    return 0;
}

} // namespace core
