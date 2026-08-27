#pragma once

#include "../../Core/Win32Lean.h"

namespace Ksword::Features::Window {

// CreateWindowFeaturePage creates the unified Window workspace. It owns one
// tab host for retained window management, clipboard inspection, capture
// protection, hierarchy diagnostics and global-hotkey probing; every page keeps
// its own actions, filters and snapshot while the user switches tabs.
HWND CreateWindowFeaturePage(HWND parent, const RECT& bounds);

} // namespace Ksword::Features::Window
