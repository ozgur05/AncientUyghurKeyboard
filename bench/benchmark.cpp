// benchmark.cpp — micro-benchmarks for the hot paths (portable, no Win32).
//
// Build target: auk_bench. Run with no arguments to print timings for UTF-16
// generation, compose-table lookup, and layout parsing. Uses steady_clock via
// core::Stopwatch. Intended for manual/CI performance tracking, not a unit test.
#include "core/Unicode.hpp"
#include "core/KeyboardLayout.hpp"
#include "core/LayoutParser.hpp"
#include "core/Stopwatch.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace {

void benchUtf16()
{
    // Mixed BMP + SMP (Old Uyghur) text, converted many times.
    std::u32string text;
    text.reserve(1000);
    for (int i = 0; i < 500; ++i) { text += char32_t(0x10F70 + (i % 24)); text += U'x'; }

    const int iters = 20000;
    volatile size_t sink = 0;
    core::Stopwatch sw;
    for (int i = 0; i < iters; ++i)
        sink += core::unicode::utf32ToUtf16(text).size();
    const double ms = sw.millis();
    std::printf("  utf32ToUtf16 : %7.2f ms  (%d x %zu cp)  => %.3f us/call\n",
                ms, iters, text.size(), (ms * 1000.0) / iters);
    (void)sink;
}

void benchCompose()
{
    core::KeyboardLayout layout;
    for (int i = 0; i < 5000; ++i) {
        std::u32string keys = { char32_t(0x10F70), char32_t(0x100000 + i) };
        layout.addCompose(core::ComposeSequence{ keys, std::u32string{ char32_t(0x30) } });
    }
    std::u32string probe = { 0x10F70 };
    const int iters = 2000000;
    volatile int sink = 0;
    core::Stopwatch sw;
    std::u32string out;
    for (int i = 0; i < iters; ++i)
        sink += static_cast<int>(layout.composeLookup(probe, out));
    const double ms = sw.millis();
    std::printf("  composeLookup: %7.2f ms  (%d probes, 5000-entry table) => %.4f us/call\n",
                ms, iters, (ms * 1000.0) / iters);
    (void)sink;
}

void benchParse()
{
    // A realistic layout document, parsed repeatedly.
    std::string json = R"({
        "meta": { "id": "b", "name": "Bench", "version": 1 },
        "behavior": { "caps_mode": "ignore", "normalize": true },
        "keys": {
            "A":"U+10F70","B":"U+10F71","C":"U+10F72","D":"U+10F73","E":"U+10F74",
            "F":"U+10F75","G":"U+10F76","H":"U+10F77","I":"U+10F78","J":"U+10F79"
        }
    })";
    const int iters = 20000;
    volatile size_t sink = 0;
    core::Stopwatch sw;
    for (int i = 0; i < iters; ++i) {
        auto r = core::LayoutParser::parseString(json);
        if (r.layout) sink += r.layout->keys().size();
    }
    const double ms = sw.millis();
    std::printf("  parseString  : %7.2f ms  (%d parses) => %.3f us/parse\n",
                ms, iters, (ms * 1000.0) / iters);
    (void)sink;
}

} // namespace

int main()
{
    std::printf("AncientUyghurKeyboard micro-benchmarks\n");
    std::printf("--------------------------------------\n");
    benchUtf16();
    benchCompose();
    benchParse();
    return 0;
}
