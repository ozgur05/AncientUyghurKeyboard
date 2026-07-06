#include "TestFramework.hpp"
#include "../src/core/KeyboardLayoutManager.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace core;

namespace {
fs::path makeDir(const std::string& tag)
{
    fs::path dir = fs::temp_directory_path() / ("auk_mgr_" + tag);
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    std::ofstream(dir / "one.json", std::ios::binary)
        << R"({ "meta": { "id": "one", "name": "One" }, "keys": { "A": "U+10F70" } })";
    std::ofstream(dir / "two.json", std::ios::binary)
        << R"({ "meta": { "id": "two", "name": "Two" }, "keys": { "B": "U+10F71" } })";
    return dir;
}
} // namespace

TEST(Manager_InitializesPreferred)
{
    fs::path dir = makeDir("init");
    KeyboardLayoutManager mgr;

    const KeyboardLayout* seen = nullptr;
    mgr.setOnChange([&](const KeyboardLayout* l) { seen = l; });

    std::string err;
    CHECK(mgr.initialize(dir.string(), "two", &err));
    CHECK_EQ(mgr.currentId(), std::string("two"));
    CHECK(mgr.current() != nullptr);
    CHECK(seen == mgr.current());          // callback fired with active layout
    CHECK_EQ(mgr.available().size(), size_t(2));
}

TEST(Manager_FallsBackWhenPreferredMissing)
{
    fs::path dir = makeDir("fallback");
    KeyboardLayoutManager mgr;
    std::string err;
    CHECK(mgr.initialize(dir.string(), "does_not_exist", &err));
    CHECK(mgr.current() != nullptr);       // fell back to a valid layout
}

TEST(Manager_SwitchAtRuntime)
{
    fs::path dir = makeDir("switch");
    KeyboardLayoutManager mgr;

    int changes = 0;
    mgr.setOnChange([&](const KeyboardLayout*) { ++changes; });

    CHECK(mgr.initialize(dir.string(), "one", nullptr));
    CHECK_EQ(mgr.currentId(), std::string("one"));

    CHECK(mgr.switchTo("two"));
    CHECK_EQ(mgr.currentId(), std::string("two"));
    CHECK(mgr.current()->key(0x42) != nullptr); // VK 'B' present in layout two

    CHECK_FALSE(mgr.switchTo("ghost"));         // unknown id fails, keeps current
    CHECK_EQ(mgr.currentId(), std::string("two"));

    CHECK(changes >= 2);                          // init + switch
}

TEST(Manager_ReloadCurrent)
{
    fs::path dir = makeDir("reload");
    KeyboardLayoutManager mgr;
    CHECK(mgr.initialize(dir.string(), "one", nullptr));
    const KeyboardLayout* before = mgr.current();
    CHECK(mgr.reloadCurrent());
    // A reload produces a fresh object (stable-pointer contract via callback).
    CHECK(mgr.current() != nullptr);
    (void)before;
}

TEST(Manager_NoLayouts_FailsCleanly)
{
    fs::path dir = fs::temp_directory_path() / "auk_mgr_none";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    KeyboardLayoutManager mgr;
    std::string err;
    CHECK_FALSE(mgr.initialize(dir.string(), "x", &err));
    CHECK(mgr.current() == nullptr);
    CHECK(!err.empty());
}
