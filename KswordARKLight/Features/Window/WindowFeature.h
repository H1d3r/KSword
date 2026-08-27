#pragma once

#include "../../Core/Win32Lean.h"

#include <string>

namespace Ksword::Features::Window {

// CreateWindowFeaturePage creates the unified Window workspace. It owns tabs
// for window management, clipboard inspection and global-hotkey probing.
// Window management owns the hierarchy diagnostics pane and exposes capture
// protection from each window row's context menu.
HWND CreateWindowFeaturePage(HWND parent, const RECT& bounds);

bool RequestWindowFeatureQuery(HWND page, const std::wstring& query);

} // namespace Ksword::Features::Window
