// KeyEditDialog.h — modal editor for one key's four modifier levels.
#pragma once

#include <windows.h>
#include "../core/KeyboardLayout.hpp"

// Edit the mappings for `vk`. `def` is in/out: seeded with the current KeyDef
// (or a default) and, on OK, updated with the edited mappings. Returns true if
// the user accepted (caller then applies via LayoutDocument), false on cancel.
bool EditKey(HWND owner, HINSTANCE hInst, unsigned vk, core::KeyDef& def);
