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

        int64_t ComputeFeeComponent(int64_t value, uint16_t basisPoints) noexcept
        {
            if (value <= 0)
            {
                return 0;
            }

            constexpr int64_t BasisPointsDivisor = 10'000;
            const int64_t rounded = (value * basisPoints + BasisPointsDivisor - 1) / BasisPointsDivisor;
            return std::max<int64_t>(1, rounded);
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

    AuctionAmount ComputeAuctionFee(const AuctionAmount& amount, uint16_t basisPoints) noexcept
    {
        if (!amount.IsFruitBasket())
        {
            return AuctionAmount::FromScalar(ComputeFeeComponent(amount.Scalar(), basisPoints));
        }

        const auto& fruits = amount.Fruits();
        return AuctionAmount::FromFruits(FruitBasket{
            ComputeFeeComponent(fruits.Strength, basisPoints),
            ComputeFeeComponent(fruits.Agility, basisPoints),
            ComputeFeeComponent(fruits.Vitality, basisPoints),
            ComputeFeeComponent(fruits.Energy, basisPoints),
            ComputeFeeComponent(fruits.Command, basisPoints),
        });
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

    std::vector<std::wstring> WalletCountCandidates(uint64_t value)
    {
        struct Unit
        {
            uint64_t size;
            wchar_t suffix;
        };
        static constexpr Unit units[] = { { 1000000000ull, L'B' }, { 1000000ull, L'M' }, { 1000ull, L'k' } };

        std::vector<std::wstring> candidates{ std::to_wstring(value) };
        for (const Unit& unit : units)
        {
            if (value < unit.size)
            {
                continue;
            }

            const uint64_t whole = value / unit.size;
            const uint64_t tenth = (value % unit.size) * 10 / unit.size;
            if (tenth != 0)
            {
                candidates.push_back(std::to_wstring(whole) + L'.' + std::to_wstring(tenth) + unit.suffix);
            }
            candidates.push_back(std::to_wstring(whole) + unit.suffix);
            break;
        }

        return candidates;
    }

    std::wstring FormatWalletCountToFit(uint64_t value, int maxWidth,
        const std::function<int(const std::wstring&)>& measureWidth)
    {
        const std::vector<std::wstring> candidates = WalletCountCandidates(value);
        for (const std::wstring& candidate : candidates)
        {
            if (measureWidth(candidate) <= maxWidth)
            {
                return candidate;
            }
        }

        return candidates.back();
    }

    int WalletRowCount(size_t cellCount, int maxColumns) noexcept
    {
        if (cellCount == 0 || maxColumns <= 0)
        {
            return 0;
        }

        const auto columns = static_cast<size_t>(maxColumns);
        return static_cast<int>((cellCount + columns - 1) / columns);
    }

    WalletCellSlot LayoutWalletCell(size_t index, size_t cellCount, int maxColumns, int panelWidth) noexcept
    {
        if (cellCount == 0 || maxColumns <= 0 || index >= cellCount)
        {
            return {};
        }

        const size_t columns = std::min(cellCount, static_cast<size_t>(maxColumns));
        const size_t row = index / columns;
        const size_t column = index % columns;
        const size_t cellsInRow = std::min(columns, cellCount - row * columns);
        const int width = panelWidth / static_cast<int>(columns);
        const int rowStart = (panelWidth - width * static_cast<int>(cellsInRow)) / 2;
        return WalletCellSlot{ static_cast<int>(row), rowStart + width * static_cast<int>(column), width };
    }

    std::vector<PriceComponent> PriceComponentsOf(AuctionCurrencyMode currency, const AuctionAmount& amount)
    {
        std::vector<PriceComponent> components;
        if (amount.IsFruitBasket())
        {
            const FruitBasket& basket = amount.Fruits();
            const PriceComponent parts[] = {
                { PriceKind::StrengthFruit, basket.Strength },
                { PriceKind::AgilityFruit, basket.Agility },
                { PriceKind::VitalityFruit, basket.Vitality },
                { PriceKind::EnergyFruit, basket.Energy },
                { PriceKind::CommandFruit, basket.Command },
            };
            for (const PriceComponent& part : parts)
            {
                if (part.amount > 0)
                {
                    components.push_back(part);
                }
            }
            return components;
        }

        const auto index = static_cast<int>(currency);
        if (amount.Scalar() > 0 && index >= 0 && index <= static_cast<int>(PriceKind::Harmony))
        {
            components.push_back({ static_cast<PriceKind>(index), amount.Scalar() });
        }
        return components;
    }

    SellDraftIssue ValidateSellDraft(const SellDraft& draft, bool buyNowSupported, int maximumComponents)
    {
        if (draft.mode == SellMode::BuyNow && !buyNowSupported)
        {
            return SellDraftIssue::BuyNowUnavailable;
        }

        if (draft.rows.empty())
        {
            return SellDraftIssue::NoPrice;
        }

        if (draft.mode == SellMode::Auction)
        {
            if (draft.rows.size() > 1)
            {
                return SellDraftIssue::TooManyComponents;
            }

            const int64_t starting = draft.rows.front().quantity;
            if (starting <= 0)
            {
                return SellDraftIssue::ZeroQuantity;
            }

            const bool hasBuyout = draft.auctionBuyoutQuantity != 0;
            return hasBuyout && draft.auctionBuyoutQuantity < starting ? SellDraftIssue::BuyoutBelowStart : SellDraftIssue::None;
        }

        if (static_cast<int>(draft.rows.size()) > maximumComponents)
        {
            return SellDraftIssue::TooManyComponents;
        }

        for (size_t i = 0; i < draft.rows.size(); ++i)
        {
            if (draft.rows[i].quantity <= 0)
            {
                return SellDraftIssue::ZeroQuantity;
            }

            for (size_t j = i + 1; j < draft.rows.size(); ++j)
            {
                if (draft.rows[i].kind == draft.rows[j].kind)
                {
                    return SellDraftIssue::DuplicateKind;
                }
            }
        }

        return SellDraftIssue::None;
    }

    std::optional<CreateFields> ToCreateFields(const SellDraft& draft)
    {
        if (draft.mode != SellMode::Auction || draft.rows.size() != 1)
        {
            return std::nullopt;
        }

        const SellPriceRow& row = draft.rows.front();
        CreateFields fields;
        if (row.kind <= PriceKind::Harmony)
        {
            fields.currencyMode = static_cast<AuctionCurrencyMode>(static_cast<int>(row.kind));
            fields.starting = AuctionAmount::FromScalar(row.quantity);
            fields.buyout = AuctionAmount::FromScalar(draft.auctionBuyoutQuantity);
            return fields;
        }

        const auto basketOf = [&row](int64_t quantity)
        {
            FruitBasket basket;
            switch (row.kind)
            {
            case PriceKind::StrengthFruit: basket.Strength = quantity; break;
            case PriceKind::AgilityFruit: basket.Agility = quantity; break;
            case PriceKind::VitalityFruit: basket.Vitality = quantity; break;
            case PriceKind::EnergyFruit: basket.Energy = quantity; break;
            default: basket.Command = quantity; break;
            }
            return basket;
        };
        fields.currencyMode = AuctionCurrencyMode::Fruits;
        fields.starting = AuctionAmount::FromFruits(basketOf(row.quantity));
        fields.buyout = AuctionAmount::FromFruits(basketOf(draft.auctionBuyoutQuantity));
        return fields;
    }

    const wchar_t* DescribeCreateResult(AuctionResult result) noexcept
    {
        switch (result)
        {
        case AuctionResult::Success: return L"Listing created.";
        case AuctionResult::Disabled: return L"The Auction House is closed right now.";
        case AuctionResult::StateRestricted: return L"You cannot list items right now.";
        case AuctionResult::LevelTooLow: return L"Your character level is too low to sell here.";
        case AuctionResult::AccountTooNew: return L"Your account is too new to sell here.";
        case AuctionResult::AccountRestricted: return L"Your account may not use the Auction House.";
        case AuctionResult::RateLimited: return L"Please wait a moment before listing again.";
        case AuctionResult::InvalidDuration: return L"That duration is not available.";
        case AuctionResult::UnsupportedCurrency:
        case AuctionResult::CurrencyConfigurationInvalid: return L"That currency cannot be used right now.";
        case AuctionResult::InvalidPrice:
        case AuctionResult::InvalidFruitBasket: return L"That price is not allowed. Check the amounts.";
        case AuctionResult::ItemNotFound: return L"The item is no longer in your inventory.";
        case AuctionResult::ItemNotEligible: return L"This item cannot be sold at the Auction House.";
        case AuctionResult::ListingLimitReached: return L"You have reached your active listing limit.";
        case AuctionResult::InsufficientCurrency: return L"You cannot afford the listing fee.";
        case AuctionResult::AlreadyProcessed: return L"That listing was already created.";
        case AuctionResult::ServiceUnavailable: return L"The Auction House is unavailable. Try again shortly.";
        default: return L"The listing could not be created.";
        }
    }

    const wchar_t* DescribeOperationResult(AuctionOperationType type, AuctionResult result) noexcept
    {
        if (type == AuctionOperationType::Create)
        {
            return DescribeCreateResult(result);
        }

        // Reasons that mean the same thing whatever the player was trying to do.
        switch (result)
        {
        case AuctionResult::Disabled: return L"The Auction House is closed right now.";
        case AuctionResult::StateRestricted: return L"You cannot do that right now.";
        case AuctionResult::LevelTooLow: return L"Your character level is too low.";
        case AuctionResult::AccountTooNew: return L"Your account is too new to use the Auction House.";
        case AuctionResult::AccountRestricted: return L"Your account may not use the Auction House.";
        case AuctionResult::RateLimited: return L"Please wait a moment and try again.";
        case AuctionResult::ServiceUnavailable: return L"The Auction House is unavailable. Try again shortly.";
        case AuctionResult::StaleListing: return L"The listing changed. Review it and try again.";
        case AuctionResult::ListingNotFound: return L"This listing no longer exists.";
        case AuctionResult::ListingNotActive:
        case AuctionResult::ListingExpired: return L"This listing has ended.";
        case AuctionResult::AlreadyProcessed: return L"That was already done.";
        default: break;
        }

        switch (type)
        {
        case AuctionOperationType::Bid:
            switch (result)
            {
            case AuctionResult::Success: return L"Bid placed.";
            case AuctionResult::BidTooLow: return L"Your bid is too low.";
            case AuctionResult::AlreadyHighestBidder: return L"You are already the highest bidder.";
            case AuctionResult::SelfTrade: return L"You cannot bid on your own listing.";
            case AuctionResult::InsufficientCurrency: return L"You do not have enough for that bid.";
            default: return L"The bid could not be placed.";
            }
        case AuctionOperationType::Buyout:
            switch (result)
            {
            case AuctionResult::Success: return L"Purchased. Collect your item from the Mailbox.";
            case AuctionResult::BuyoutUnavailable: return L"This listing has no buyout.";
            case AuctionResult::SelfTrade: return L"You cannot buy your own listing.";
            case AuctionResult::InsufficientCurrency: return L"You do not have enough to buy this.";
            default: return L"The purchase could not be completed.";
            }
        case AuctionOperationType::Cancel:
            switch (result)
            {
            case AuctionResult::Success: return L"Listing cancelled. The item is in your Mailbox.";
            case AuctionResult::CannotCancelWithBid: return L"A listing with bids cannot be cancelled.";
            default: return L"The listing could not be cancelled.";
            }
        default:
            return result == AuctionResult::Success ? L"Done." : L"That could not be completed.";
        }
    }

    std::optional<std::vector<PriceChipPlacement>> PackPriceChips(
        const std::vector<int>& chipWidths, int lineWidth, int gap, int maxLines)
    {
        std::vector<PriceChipPlacement> placements;
        placements.reserve(chipWidths.size());

        int line = 0;
        int lineUsed = 0;
        size_t lineStart = 0;
        const auto centreLine = [&](size_t end)
        {
            const int offset = (lineWidth - lineUsed) / 2;
            for (size_t i = lineStart; i < end; ++i)
            {
                placements[i].x += offset;
            }
        };

        for (size_t i = 0; i < chipWidths.size(); ++i)
        {
            const int width = chipWidths[i];
            if (width > lineWidth)
            {
                return std::nullopt;
            }

            const bool startsLine = i == lineStart;
            if (!startsLine && lineUsed + gap + width > lineWidth)
            {
                centreLine(i);
                ++line;
                if (line >= maxLines)
                {
                    return std::nullopt;
                }
                lineStart = i;
                lineUsed = 0;
            }

            const int x = i == lineStart ? 0 : lineUsed + gap;
            placements.push_back({ line, x });
            lineUsed = x + width;
        }

        centreLine(chipWidths.size());
        return placements;
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

    int SideBySideLeft(int desiredLeft, int width, int rightLimit, int minLeft) noexcept
    {
        const int overlap = desiredLeft + width - rightLimit;
        const int left = overlap > 0 ? desiredLeft - overlap : desiredLeft;
        return left < minLeft ? minLeft : left;
    }
}
