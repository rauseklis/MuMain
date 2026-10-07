// <copyright file="AuctionModel.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "AuctionModel.h"

#include <algorithm>
#include <atomic>
#include <random>

namespace AuctionHouse
{
    AuctionSort NextBrowseSort(AuctionBrowseSortColumn column, AuctionSort current) noexcept
    {
        if (column == AuctionBrowseSortColumn::TimeLeft)
        {
            return AuctionSort::EndingSoonest;
        }

        return current == AuctionSort::PriceAscending ? AuctionSort::PriceDescending : AuctionSort::PriceAscending;
    }

    AuctionSort NormalizeBrowseSort(AuctionCurrencyMode currency, AuctionSort sort) noexcept
    {
        if (currency == AuctionCurrencyMode::Fruits
            && (sort == AuctionSort::PriceAscending || sort == AuctionSort::PriceDescending))
        {
            return AuctionSort::EndingSoonest;
        }

        return sort;
    }

    std::string ClampUtf8ToByteCapacity(std::string_view utf8, size_t capacity)
    {
        if (utf8.size() <= capacity)
        {
            return std::string(utf8);
        }

        size_t length = capacity;
        while (length > 0 && (static_cast<unsigned char>(utf8[length]) & 0xC0u) == 0x80u)
        {
            --length;
        }

        return std::string(utf8.substr(0, length));
    }

    uint8_t EncodeListingStatusFilter(int selectedIndex) noexcept
    {
        return selectedIndex <= 0 ? 0xFFu : static_cast<uint8_t>(selectedIndex - 1);
    }

    uint8_t EncodeCollectionKindFilter(int selectedIndex) noexcept
    {
        return selectedIndex <= 0 ? 0xFFu : static_cast<uint8_t>(selectedIndex - 1);
    }

    bool IsValidCollectionClaim(AuctionCurrencyMode currency, const AuctionAmount& remaining,
        const AuctionAmount& requested) noexcept
    {
        if (currency != AuctionCurrencyMode::Fruits)
        {
            return !remaining.IsFruitBasket() && !requested.IsFruitBasket()
                && requested.Scalar() >= 1 && requested.Scalar() <= remaining.Scalar();
        }

        if (!remaining.IsFruitBasket() || !requested.IsFruitBasket() || requested.IsZero())
        {
            return false;
        }

        const auto& available = remaining.Fruits();
        const auto& claim = requested.Fruits();
        return claim.Strength >= 0 && claim.Strength <= available.Strength
            && claim.Agility >= 0 && claim.Agility <= available.Agility
            && claim.Vitality >= 0 && claim.Vitality <= available.Vitality
            && claim.Energy >= 0 && claim.Energy <= available.Energy
            && claim.Command >= 0 && claim.Command <= available.Command;
    }

    bool CanCancelOwnedListing(AuctionListingStatus status, uint16_t bidCount) noexcept
    {
        return status == AuctionListingStatus::Active && bidCount == 0;
    }

    size_t MaximumBrowseScrollOffset(size_t listingCount, size_t visibleRows) noexcept
    {
        return listingCount > visibleRows ? listingCount - visibleRows : 0;
    }

    namespace
    {
        // ceil(value * 5 / 100) using only integer arithmetic, since prices are whole units and this must
        // match the server's own rounding exactly (no floating point).
        int64_t CeilFivePercent(int64_t value) noexcept
        {
            return (value * 5 + 99) / 100;
        }
    }

    AuctionAmount ComputeMinimumNextBid(AuctionCurrencyMode mode, const AuctionAmount& currentPrice, uint16_t bidCount) noexcept
    {
        if (bidCount == 0)
        {
            return currentPrice;
        }

        if (mode == AuctionCurrencyMode::Fruits)
        {
            const auto& current = currentPrice.Fruits();
            const auto bump = [](int64_t component) noexcept -> int64_t
            {
                return component == 0 ? 0 : component + std::max<int64_t>(1, CeilFivePercent(component));
            };

            return AuctionAmount::FromFruits(FruitBasket{
                bump(current.Strength),
                bump(current.Agility),
                bump(current.Vitality),
                bump(current.Energy),
                bump(current.Command),
            });
        }

        const int64_t minimumFloor = (mode == AuctionCurrencyMode::Zen) ? 1000 : 1;
        const int64_t current = currentPrice.Scalar();
        return AuctionAmount::FromScalar(current + std::max(minimumFloor, CeilFivePercent(current)));
    }

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
