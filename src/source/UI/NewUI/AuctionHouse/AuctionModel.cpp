// <copyright file="AuctionModel.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "AuctionModel.h"

namespace AuctionHouse
{
    std::wstring FormatAuctionCountdown(std::chrono::seconds remaining)
    {
        if (remaining <= std::chrono::seconds::zero())
        {
            return L"Ended";
        }

        constexpr auto SecondsPerMinute = 60;
        constexpr auto SecondsPerHour = 60 * SecondsPerMinute;
        constexpr auto SecondsPerDay = 24 * SecondsPerHour;

        const auto total = remaining.count();
        if (total >= SecondsPerDay)
        {
            return std::to_wstring(total / SecondsPerDay) + L"d " + std::to_wstring((total % SecondsPerDay) / SecondsPerHour) + L"h";
        }

        if (total >= SecondsPerHour)
        {
            return std::to_wstring(total / SecondsPerHour) + L"h " + std::to_wstring((total % SecondsPerHour) / SecondsPerMinute) + L"m";
        }

        if (total >= SecondsPerMinute)
        {
            return std::to_wstring(total / SecondsPerMinute) + L"m " + std::to_wstring(total % SecondsPerMinute) + L"s";
        }

        return std::to_wstring(total) + L"s";
    }
}
