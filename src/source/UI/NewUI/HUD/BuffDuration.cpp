#include "UI/NewUI/HUD/BuffDuration.h"

#include <cwchar>

namespace UI::Buffs
{
    std::wstring FormatDuration(std::uint64_t expiresAt, std::uint64_t now)
    {
        constexpr std::uint64_t MillisecondsPerSecond = 1000;
        constexpr std::uint64_t SecondsPerMinute = 60;
        constexpr std::uint64_t SecondsPerHour = 3600;
        constexpr std::uint64_t SecondsPerDay = 86400;
        const auto seconds = expiresAt > now ? (expiresAt - now + MillisecondsPerSecond - 1) / MillisecondsPerSecond : 0;
        const auto days = seconds / SecondsPerDay;
        wchar_t clock[32]{};
        std::swprintf(clock, sizeof(clock) / sizeof(clock[0]), L"%02llu:%02llu:%02llu",
            (seconds % SecondsPerDay) / SecondsPerHour,
            (seconds / SecondsPerMinute) % SecondsPerMinute, seconds % SecondsPerMinute);
        return L"Remaining: " + (days > 0 ? std::to_wstring(days) + L"d " : std::wstring{}) + clock;
    }
}
