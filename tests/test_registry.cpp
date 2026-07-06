#include "TestFramework.hpp"
#include "../src/core/LayoutRegistry.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace core;

namespace {
// Create an isolated temp directory with sample layout files.
fs::path makeSampleDir(const std::string& tag)
{
    fs::path dir = fs::temp_directory_path() / ("auk_reg_" + tag);
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    auto write = [&](const char* file, const char* body) {
        std::ofstream(dir / file, std::ios::binary) << body;
    };
    write("alpha.json", R"({ "meta": { "id": "alpha", "name": "Alpha" },
                             "keys": { "A": "U+10F70" } })");
    write("beta.json",  R"({ "meta": { "name": "Beta" },
                             "keys": { "B": "U+10F71" } })");
    write("broken.json", R"({ "meta": { "name": "Broken" } })"); // no keys
    return dir;
}
} // namespace

TEST(Registry_ScansAndLists)
{
    fs::path dir = makeSampleDir("scan");
    LayoutRegistry reg;
    size_t n = reg.scan(dir.string());
    CHECK_EQ(n, size_t(3));
    CHECK_EQ(reg.layouts().size(), size_t(3));
}

TEST(Registry_IdFromMetaOrStem)
{
    fs::path dir = makeSampleDir("id");
    LayoutRegistry reg;
    reg.scan(dir.string());
    // alpha.json declares meta.id "alpha"; beta.json falls back to file stem.
    CHECK(reg.find("alpha") != nullptr);
    CHECK(reg.find("beta") != nullptr);
}

TEST(Registry_MarksInvalid)
{
    fs::path dir = makeSampleDir("invalid");
    LayoutRegistry reg;
    reg.scan(dir.string());
    const LayoutInfo* broken = reg.find("broken");
    CHECK(broken != nullptr);
    if (broken) CHECK_FALSE(broken->valid);
}

TEST(Registry_LoadValidAndInvalid)
{
    fs::path dir = makeSampleDir("load");
    LayoutRegistry reg;
    reg.scan(dir.string());

    auto good = reg.load("alpha");
    CHECK(good.ok());
    if (good.layout) CHECK(good.layout->key(0x41) != nullptr); // VK 'A'

    auto bad = reg.load("broken");
    CHECK_FALSE(bad.ok());

    auto missing = reg.load("nope");
    CHECK_FALSE(missing.ok());
}

TEST(Registry_EmptyDir)
{
    fs::path dir = fs::temp_directory_path() / "auk_reg_empty";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    LayoutRegistry reg;
    CHECK_EQ(reg.scan(dir.string()), size_t(0));
}
