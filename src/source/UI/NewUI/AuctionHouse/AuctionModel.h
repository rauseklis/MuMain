// <copyright file="AuctionModel.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "Dotnet/PacketFunctions_CommonEnums.h"

namespace AuctionHouse
{
    enum class AuctionBrowseSortColumn
    {
        TimeLeft,
        Price,
    };

    // Maps the two sortable Browse headers onto the four sort values approved by the wire protocol. Price
    // toggles in both directions; Time Left always restores the truthful earliest-expiry-first order.
    [[nodiscard]] AuctionSort NextBrowseSort(AuctionBrowseSortColumn column, AuctionSort current) noexcept;

    // Scalar price ordering is undefined for component-wise fruit baskets and the server rejects it. Switching
    // currency therefore restores the safe time sort before the next request is sent.
    [[nodiscard]] AuctionSort NormalizeBrowseSort(AuctionCurrencyMode currency, AuctionSort sort) noexcept;

    // The browse packet reserves a fixed byte array, not a character array. Input arrives here as valid UTF-8;
    // clamp it without leaving a partial multi-byte code point at the end of the packet field.
    [[nodiscard]] std::string ClampUtf8ToByteCapacity(std::string_view utf8, size_t capacity);

    // Combo index zero means every status (the protocol's 0xFF sentinel); the remaining entries are the
    // contiguous AuctionListingStatus values in display order.
    [[nodiscard]] uint8_t EncodeListingStatusFilter(int selectedIndex) noexcept;

    // Index zero is the Mailbox UI's "all kinds" sentinel; following entries match the contiguous wire enum.
    [[nodiscard]] uint8_t EncodeCollectionKindFilter(int selectedIndex) noexcept;

    // Mirrors the design's seller-side cancellation eligibility for UI guidance. The server remains
    // authoritative and re-checks status, ownership, version and bid count under the listing lock.
    [[nodiscard]] bool CanCancelOwnedListing(AuctionListingStatus status, uint16_t bidCount) noexcept;

    // Returns the number of rows hidden above an eight-row-style Browse viewport when scrolled fully to the
    // bottom. The result is zero when every listing already fits, avoiding unsigned underflow for short pages.
    [[nodiscard]] size_t MaximumBrowseScrollOffset(size_t listingCount, size_t visibleRows) noexcept;

    // The five fruit components of a Fruits-currency listing or bid. Zero in every field means "no fruits set".
    struct FruitBasket
    {
        int64_t Strength = 0;
        int64_t Agility = 0;
        int64_t Vitality = 0;
        int64_t Energy = 0;
        int64_t Command = 0;

        [[nodiscard]] bool IsZero() const noexcept
        {
            return this->Strength == 0 && this->Agility == 0 && this->Vitality == 0 && this->Energy == 0 && this->Command == 0;
        }
    };

    [[nodiscard]] inline bool operator==(const FruitBasket& left, const FruitBasket& right) noexcept
    {
        return left.Strength == right.Strength && left.Agility == right.Agility && left.Vitality == right.Vitality
            && left.Energy == right.Energy && left.Command == right.Command;
    }

    [[nodiscard]] inline FruitBasket operator+(const FruitBasket& left, const FruitBasket& right) noexcept
    {
        return FruitBasket{
            left.Strength + right.Strength,
            left.Agility + right.Agility,
            left.Vitality + right.Vitality,
            left.Energy + right.Energy,
            left.Command + right.Command,
        };
    }

    // A listing/bid/buyout amount: a single scalar unit count for Zen or a jewel currency, or a fruit basket for the Fruits
    // currency. Prices never compare across currencies, so an amount is always read together with its listing's currency.
    class AuctionAmount
    {
    public:
        [[nodiscard]] static AuctionAmount FromScalar(int64_t units) noexcept
        {
            AuctionAmount amount;
            amount._isFruitBasket = false;
            amount._scalar = units;
            return amount;
        }

        [[nodiscard]] static AuctionAmount FromFruits(const FruitBasket& basket) noexcept
        {
            AuctionAmount amount;
            amount._isFruitBasket = true;
            amount._fruits = basket;
            return amount;
        }

        [[nodiscard]] bool IsFruitBasket() const noexcept { return this->_isFruitBasket; }

        [[nodiscard]] int64_t Scalar() const noexcept { return this->_scalar; }

        [[nodiscard]] const FruitBasket& Fruits() const noexcept { return this->_fruits; }

        [[nodiscard]] bool IsZero() const noexcept
        {
            return this->_isFruitBasket ? this->_fruits.IsZero() : this->_scalar == 0;
        }

    private:
        bool _isFruitBasket = false;
        int64_t _scalar = 0;
        FruitBasket _fruits{};
    };

    // Client-side guidance for partial Mailbox claims. Jewel quantities must be within the remaining scalar;
    // fruit requests may omit components but must request at least one and never exceed any remaining part.
    // The server independently validates the same invariant under the collection lock.
    [[nodiscard]] bool IsValidCollectionClaim(AuctionCurrencyMode currency, const AuctionAmount& remaining,
        const AuctionAmount& requested) noexcept;

    // Whether a currency mode settles in a fruit basket rather than a single scalar unit count.
    [[nodiscard]] inline bool IsFruitCurrency(AuctionCurrencyMode mode) noexcept
    {
        return mode == AuctionCurrencyMode::Fruits;
    }

    // The minimum amount a new bid on `currentPrice` must reach to be valid, mirroring the server's own
    // bidding rule (design spec: a first bid must meet the starting price; a later Zen bid must add at least
    // max(1000, ceil(5%)); a later jewel bid must add at least max(1, ceil(5%)); a later fruit bid must add at
    // least max(1, ceil(5%)) to every already-nonzero component, never introducing or removing one). This is
    // advisory only, for a quick-bid control to offer a correct starting value — the server independently
    // re-validates every bid, so duplicating this formula client-side is not a trust boundary.
    [[nodiscard]] AuctionAmount ComputeMinimumNextBid(AuctionCurrencyMode mode, const AuctionAmount& currentPrice, uint16_t bidCount) noexcept;

    // Advisory listing/success fee preview. Mirrors the server's whole-unit calculation: multiply by the
    // configured basis-point rate, round upward, and charge at least one unit for every positive scalar or
    // fruit component. Fruit fees are calculated independently; zero components stay zero.
    [[nodiscard]] AuctionAmount ComputeAuctionFee(const AuctionAmount& amount, uint16_t basisPoints) noexcept;

    // Formats the time remaining on a listing for the countdown display: "Ended" once it has reached zero, otherwise the two
    // most significant units ("1d 1h", "1h 1m", "2m 5s", or "45s" under a minute).
    [[nodiscard]] std::wstring FormatAuctionCountdown(std::chrono::seconds remaining);

    // Tracks the offset between the server's reported time (seconds, from a response's ServerTime field) and
    // this client's own GetTickCount()-style local tick clock, so a listing's remaining time can tick down
    // locally between responses instead of only updating on the next server reply. Local tick values are
    // expected to be unsigned milliseconds that wrap on overflow, the same contract GetTickCount() itself has;
    // unsigned subtraction keeps the estimate correct across that wraparound, the same property every
    // GetTickCount() caller elsewhere in this codebase already relies on.
    class AuctionServerClock
    {
    public:
        // Call whenever a response carrying ServerTime arrives, with the client's own tick count at that moment.
        void Sync(uint32_t serverTimeSeconds, uint32_t localTickMs) noexcept
        {
            this->_serverTimeSeconds = serverTimeSeconds;
            this->_syncTickMs = localTickMs;
            this->_hasSynced = true;
        }

        [[nodiscard]] bool HasSynced() const noexcept { return this->_hasSynced; }

        // Estimates the current server time given the client's current tick count. Zero if never synced.
        [[nodiscard]] uint32_t EstimatedServerTime(uint32_t localTickMs) const noexcept
        {
            if (!this->_hasSynced)
            {
                return 0;
            }

            const uint32_t elapsedMs = localTickMs - this->_syncTickMs;
            return this->_serverTimeSeconds + elapsedMs / 1000;
        }

    private:
        bool _hasSynced = false;
        uint32_t _serverTimeSeconds = 0;
        uint32_t _syncTickMs = 0;
    };

    // Returns the next request id for correlating a query request (open/browse/detail/my-listings/mailbox)
    // with its response. Monotonically increasing within one process run; not persisted, not thread-safe
    // (this client's network/UI code runs on a single thread, same as the rest of NewUI).
    [[nodiscard]] uint32_t NextAuctionRequestId() noexcept;

    // Generates a new 16-byte operation id for a mutation request (create/bid/buyout/cancel/collect), echoed
    // back by the server on the matching AuctionOperationResponse so a reply can be matched to its request and
    // a retried request recognized as a duplicate. Not a spec-compliant RFC 4122 UUID (no version/variant bits
    // are set) since the server only needs practical per-account uniqueness, not interoperability with other
    // UUID consumers.
    [[nodiscard]] std::array<uint8_t, 16> GenerateAuctionOperationId();
}
