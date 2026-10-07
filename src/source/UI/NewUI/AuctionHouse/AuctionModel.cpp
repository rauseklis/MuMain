// <copyright file="AuctionModel.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "AuctionModel.h"

#include <atomic>
#include <random>

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

    uint32_t NextAuctionRequestId() noexcept
    {
        // Starts at 1 so a default-constructed/zeroed request id never collides with a real one.
        static std::atomic<uint32_t> counter{ 1 };
        return counter.fetch_add(1, std::memory_order_relaxed);
    }

    std::array<uint8_t, 16> GenerateAuctionOperationId()
    {
        // Seeded once per process from std::random_device, matching the project's own Random::GetThreadEngine
        // seeding approach (Core/Utilities/Random.cpp) but kept self-contained here: that file includes the
        // engine's precompiled header, which this pure-logic translation unit deliberately does not.
        static std::mt19937_64 engine{ std::random_device{}() };
        static std::uniform_int_distribution<int> byteDistribution(0, 255);

        std::array<uint8_t, 16> id{};
        for (auto& byte : id)
        {
            byte = static_cast<uint8_t>(byteDistribution(engine));
        }

        return id;
    }
}
