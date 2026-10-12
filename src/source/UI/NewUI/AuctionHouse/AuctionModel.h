// <copyright file="AuctionModel.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

    // Returns the number of rows hidden above the Browse viewport when scrolled fully to the
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

    // Wallet cards show how many jewels or fruits the player holds. The count has to stay readable once it
    // reaches the thousands (a jewel bank holds far more than an inventory), so it is shown as its exact digits
    // when they fit and otherwise as a truncated compact form: "12.3k", "1.5M", "2B". Truncated, never
    // rounded, so a card can never claim more than the player has. Candidates run from the exact digits to
    // the shortest form.
    [[nodiscard]] std::vector<std::wstring> WalletCountCandidates(uint64_t value);

    // The most precise candidate whose measured width fits maxWidth; the shortest one when none does.
    // measureWidth is injected so the caller supplies the real font's metrics.
    [[nodiscard]] std::wstring FormatWalletCountToFit(uint64_t value, int maxWidth,
        const std::function<int(const std::wstring&)>& measureWidth);

    // Where one cell of a wallet grid sits inside its panel, relative to the panel's left edge. Cells fill
    // rows of up to maxColumns; a short last row is centred under the full ones.
    struct WalletCellSlot
    {
        int row = 0;
        int x = 0;
        int width = 0;
    };

    [[nodiscard]] int WalletRowCount(size_t cellCount, int maxColumns) noexcept;
    [[nodiscard]] WalletCellSlot LayoutWalletCell(size_t index, size_t cellCount, int maxColumns, int panelWidth) noexcept;

    // What a price is made of. A listing's asking price is a list of components - an amount of Zen, of a jewel,
    // of one stat fruit - so a price paid in several things at once reads as exactly what it is, one chip per
    // component, instead of a label standing in for the mix. The first eight kinds match the scalar currency
    // modes in order; the five fruits follow in the order of FruitBasket's fields.
    enum class PriceKind
    {
        Zen,
        Chaos,
        Bless,
        Soul,
        Life,
        Creation,
        Guardian,
        Harmony,
        StrengthFruit,
        AgilityFruit,
        VitalityFruit,
        EnergyFruit,
        CommandFruit,
    };

    struct PriceComponent
    {
        PriceKind kind = PriceKind::Zen;
        int64_t amount = 0;
    };

    // The nonzero components of a price: one for a scalar price, one per nonzero fruit for a fruit price,
    // none when the price is zero (no buyout set, for instance).
    [[nodiscard]] std::vector<PriceComponent> PriceComponentsOf(AuctionCurrencyMode currency, const AuctionAmount& amount);

    // Where one price chip sits in a price cell: its line, and its x relative to the cell's left edge.
    struct PriceChipPlacement
    {
        int line = 0;
        int x = 0;
    };

    // Packs chips of the given widths left to right into at most maxLines lines of lineWidth, gap apart, and
    // centres each line on its own. Returns nothing when a chip is wider than a line or the chips need more
    // than maxLines lines, so the caller can retry with shorter text instead of overflowing the cell.
    [[nodiscard]] std::optional<std::vector<PriceChipPlacement>> PackPriceChips(
        const std::vector<int>& chipWidths, int lineWidth, int gap, int maxLines);

    // How a seller lists an item. An auction has a single-component starting price, an optional buyout of the same
    // kind, and takes bids. A buy-now listing has only a fixed price of one to five components and no bids; a price
    // paid in several things at once is always buy now, because bids on a mix could not be ranked.
    enum class SellMode
    {
        Auction,
        BuyNow,
    };

    struct SellPriceRow
    {
        PriceKind kind = PriceKind::Zen;
        int64_t quantity = 0;
    };

    struct SellDraft
    {
        SellMode mode = SellMode::Auction;
        // Auction: the one starting-price row. Buy now: every component of the fixed price.
        std::vector<SellPriceRow> rows;
        // Auction only: a buyout in the starting price's kind; zero means no buyout.
        int64_t auctionBuyoutQuantity = 0;
        uint8_t durationHours = 24;
    };

    enum class SellDraftIssue
    {
        None,
        NoPrice,
        ZeroQuantity,
        TooManyComponents,
        DuplicateKind,
        BuyoutBelowStart,
        BuyNowUnavailable,
    };

    inline constexpr int MaximumSellComponents = 5;

    // Advisory client-side check of a draft, so the Create button can say why it is disabled. The server remains
    // the authority on ranges, eligibility and fees. buyNowSupported is false while the server's protocol only has
    // single-currency listings.
    [[nodiscard]] SellDraftIssue ValidateSellDraft(
        const SellDraft& draft, bool buyNowSupported, int maximumComponents = MaximumSellComponents);

    // The fields of the current create request, which has one currency mode with a scalar or fruit-basket price.
    struct CreateFields
    {
        AuctionCurrencyMode currencyMode = AuctionCurrencyMode::Zen;
        AuctionAmount starting = AuctionAmount::FromScalar(0);
        AuctionAmount buyout = AuctionAmount::FromScalar(0);
    };

    // Maps an auction draft onto the create request: Zen and jewels as scalars, a stat fruit as a fruit basket holding
    // that one fruit. A buy-now draft has no representation yet and yields nothing.
    [[nodiscard]] std::optional<CreateFields> ToCreateFields(const SellDraft& draft);

    // The message shown to the seller for the outcome of a create-listing request.
    [[nodiscard]] const wchar_t* DescribeCreateResult(AuctionResult result) noexcept;

    // The message shown for the outcome of a bid, buyout, cancellation or create. Collect results have their own
    // localized texts in the window because they carry claimed and remaining amounts.
    [[nodiscard]] const wchar_t* DescribeOperationResult(AuctionOperationType type, AuctionResult result) noexcept;

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
