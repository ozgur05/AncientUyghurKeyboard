// Stopwatch.hpp — portable high-resolution timing for diagnostics/benchmarks.
//
// Header-only, Win32-free (uses std::chrono::steady_clock). Used by the startup
// timing diagnostics and the benchmark utility.
#pragma once

#include <chrono>
#include <cstdint>

namespace core {

class Stopwatch {
public:
    Stopwatch() : m_start(clock::now()) {}

    void reset() { m_start = clock::now(); }

    // Elapsed time since construction / last reset.
    double millis() const {
        return std::chrono::duration<double, std::milli>(clock::now() - m_start).count();
    }
    double micros() const {
        return std::chrono::duration<double, std::micro>(clock::now() - m_start).count();
    }
    int64_t nanos() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   clock::now() - m_start).count();
    }

private:
    using clock = std::chrono::steady_clock;
    clock::time_point m_start;
};

} // namespace core
