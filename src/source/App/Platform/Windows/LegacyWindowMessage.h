#pragma once

#include "Core/Platform/WinCompat.h"

namespace App::Platform
{
// SDL owns native window lifecycle, system commands and default processing.
// Also preserve the game's synthetic WM_DESTROY shutdown requests (e.g. offlevel).
// Observed messages must never invoke duplicate native default processing.
inline bool ShouldForwardLegacyWindowMessage(UINT message, UINT protectionExitMessage)
{
    return message == WM_DESTROY || message == WM_SYSKEYDOWN || message == WM_SETCURSOR ||
           message == WM_IME_NOTIFY || message == WM_CHAR || message == protectionExitMessage;
}
} // namespace App::Platform
