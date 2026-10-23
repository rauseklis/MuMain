#pragma once

#include "Core/Platform/WinCompat.h"

namespace App::Platform
{
// SDL owns native window lifecycle, system commands and default processing.
// This hook observes only the legacy input and protection messages still used
// by the game; forwarding every message calls DefWindowProc before SDL does.
inline bool ShouldForwardLegacyWindowMessage(UINT message, UINT protectionExitMessage)
{
    return message == WM_SYSKEYDOWN || message == WM_SETCURSOR || message == WM_IME_NOTIFY ||
           message == WM_CHAR || message == protectionExitMessage;
}
} // namespace App::Platform
