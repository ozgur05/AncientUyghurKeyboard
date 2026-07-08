// Modal.h — run a child popup window as a modal dialog (no .rc templates).
//
// Disables the owner, pumps messages (with IsDialogMessage for Tab navigation)
// until `done` becomes true, then restores the owner. Keeps the designer free
// of binary dialog templates, which cannot be verified without running the GUI.
#pragma once

#include <windows.h>

inline void RunModal(HWND owner, HWND dlg, const bool& done)
{
    if (owner) EnableWindow(owner, FALSE);
    ShowWindow(dlg, SW_SHOW);
    SetForegroundWindow(dlg);

    MSG msg;
    while (!done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (owner) {
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
    }
    DestroyWindow(dlg);
}
