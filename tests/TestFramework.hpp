// TestFramework.hpp — a minimal, dependency-free test harness.
//
// Register tests with TEST(name){...}; assert with CHECK / CHECK_EQ.
// A failed check records a message and marks the test failed but keeps going.
// main() is provided by RUN_ALL_TESTS() in test_main.cpp.
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

namespace testing {

struct Case {
    std::string name;
    std::function<void(struct Context&)> fn;
};

struct Context {
    int failures = 0;
    std::vector<std::string> messages;

    void fail(const std::string& expr, const std::string& file, int line,
              const std::string& extra = "")
    {
        ++failures;
        std::ostringstream o;
        o << "    FAIL " << file << ":" << line << "  " << expr;
        if (!extra.empty()) o << "  (" << extra << ")";
        messages.push_back(o.str());
    }
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void(Context&)> fn) {
        registry().push_back({ name, std::move(fn) });
    }
};

inline int runAll()
{
    int failed = 0, total = 0;
    for (auto& c : registry()) {
        ++total;
        Context ctx;
        try {
            c.fn(ctx);
        } catch (const std::exception& e) {
            ctx.fail("threw std::exception", "?", 0, e.what());
        } catch (...) {
            ctx.fail("threw unknown exception", "?", 0);
        }
        if (ctx.failures == 0) {
            std::cout << "[ PASS ] " << c.name << "\n";
        } else {
            ++failed;
            std::cout << "[ FAIL ] " << c.name << "\n";
            for (auto& m : ctx.messages) std::cout << m << "\n";
        }
    }
    std::cout << "\n" << (total - failed) << "/" << total << " tests passed.\n";
    return failed == 0 ? 0 : 1;
}

} // namespace testing

#define TEST(NAME)                                                            \
    static void NAME(testing::Context& _ctx);                                 \
    static testing::Registrar _reg_##NAME(#NAME, NAME);                       \
    static void NAME(testing::Context& _ctx)

#define CHECK(COND)                                                           \
    do { if (!(COND)) _ctx.fail(#COND, __FILE__, __LINE__); } while (0)

#define CHECK_EQ(A, B)                                                        \
    do {                                                                      \
        auto _a = (A); auto _b = (B);                                         \
        if (!(_a == _b)) {                                                    \
            std::ostringstream _o; _o << "got != expected";                   \
            _ctx.fail(#A " == " #B, __FILE__, __LINE__, _o.str());            \
        }                                                                     \
    } while (0)

#define CHECK_TRUE(COND)  CHECK(COND)
#define CHECK_FALSE(COND) CHECK(!(COND))
