#include "doctest.h"
#include "App/Platform/Windows/LegacyWindowMessage.h"

#include <array>

namespace
{
constexpr UINT ProtectionExitMessage = WM_USER + 1000;
}

TEST_CASE("SDL alone processes native window lifecycle messages [window]")
{
    // WM_SYSCOMMAND is 0x0112 in the Windows message protocol.
    const std::array<UINT, 6> messages = {WM_ACTIVATE, WM_SIZE, WM_PAINT, WM_CLOSE, WM_ERASEBKGND, 0x0112};

    for (const UINT message : messages)
    {
        INFO("message: ", message);
        CHECK_FALSE(App::Platform::ShouldForwardLegacyWindowMessage(message, ProtectionExitMessage));
    }
}

TEST_CASE("Legacy input and protection messages still reach the game [window]")
{
    const std::array<UINT, 5> messages = {WM_SYSKEYDOWN, WM_SETCURSOR, WM_IME_NOTIFY, WM_CHAR, ProtectionExitMessage};

    for (const UINT message : messages)
    {
        INFO("message: ", message);
        CHECK(App::Platform::ShouldForwardLegacyWindowMessage(message, ProtectionExitMessage));
    }
}
