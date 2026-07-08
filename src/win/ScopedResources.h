// ScopedResources.h — small RAII wrappers for Win32 resources.
//
// Prevents handle/menu leaks and guarantees cleanup on every return path,
// including exceptions. Move-only; never copyable.
#pragma once

#include <windows.h>
#include <utility>

// Generic HANDLE (mutex, event, file, …) closed with CloseHandle.
class ScopedHandle {
public:
    ScopedHandle() = default;
    explicit ScopedHandle(HANDLE h) : m_h(h) {}
    ~ScopedHandle() { reset(); }

    ScopedHandle(ScopedHandle&& o) noexcept : m_h(o.m_h) { o.m_h = nullptr; }
    ScopedHandle& operator=(ScopedHandle&& o) noexcept {
        if (this != &o) { reset(); m_h = o.m_h; o.m_h = nullptr; }
        return *this;
    }
    ScopedHandle(const ScopedHandle&)            = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    HANDLE get() const { return m_h; }
    bool   valid() const { return m_h != nullptr && m_h != INVALID_HANDLE_VALUE; }

    void reset(HANDLE h = nullptr) {
        if (valid()) CloseHandle(m_h);
        m_h = h;
    }

private:
    HANDLE m_h = nullptr;
};

// HMENU destroyed with DestroyMenu. Scoped to a single popup/track cycle.
class ScopedMenu {
public:
    ScopedMenu() : m_menu(CreatePopupMenu()) {}
    ~ScopedMenu() { if (m_menu) DestroyMenu(m_menu); }

    ScopedMenu(ScopedMenu&& o) noexcept : m_menu(o.m_menu) { o.m_menu = nullptr; }
    ScopedMenu& operator=(ScopedMenu&& o) noexcept {
        if (this != &o) { if (m_menu) DestroyMenu(m_menu); m_menu = o.m_menu; o.m_menu = nullptr; }
        return *this;
    }
    ScopedMenu(const ScopedMenu&)            = delete;
    ScopedMenu& operator=(const ScopedMenu&) = delete;

    HMENU get() const { return m_menu; }
    explicit operator bool() const { return m_menu != nullptr; }

private:
    HMENU m_menu = nullptr;
};
