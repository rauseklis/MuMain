#include <doctest.h>

#include <optional>
#include <string>
#include <vector>

#include "UI/NewUI/AuctionHouse/AuctionModel.h"

using namespace AuctionHouse;

TEST_CASE("a zero fruit basket reports IsZero [ui][auction_model]")
{
    CHECK(FruitBasket{}.IsZero());
}

TEST_CASE("a fruit basket with any one component set is not zero [ui][auction_model]")
{
    CHECK_FALSE(FruitBasket{ .Command = 1 }.IsZero());
}

TEST_CASE("fruit baskets add component-wise [ui][auction_model]")
{
    const FruitBasket left{ .Strength = 1, .Agility = 2, .Vitality = 3, .Energy = 4, .Command = 5 };
    const FruitBasket right{ .Strength = 10, .Agility = 20, .Vitality = 30, .Energy = 40, .Command = 50 };

    const auto sum = left + right;

    CHECK(sum == FruitBasket{ .Strength = 11, .Agility = 22, .Vitality = 33, .Energy = 44, .Command = 55 });
}

TEST_CASE("a scalar amount is not a fruit basket and reports its units [ui][auction_model]")
{
    const auto amount = AuctionAmount::FromScalar(1000);

    CHECK_FALSE(amount.IsFruitBasket());
    CHECK(amount.Scalar() == 1000);
}

TEST_CASE("a fruit amount reports its basket [ui][auction_model]")
{
    const FruitBasket basket{ .Strength = 7 };
    const auto amount = AuctionAmount::FromFruits(basket);

    CHECK(amount.IsFruitBasket());
    CHECK(amount.Fruits() == basket);
}

TEST_CASE("a zero scalar amount is zero [ui][auction_model]")
{
    CHECK(AuctionAmount::FromScalar(0).IsZero());
    CHECK_FALSE(AuctionAmount::FromScalar(1).IsZero());
}

TEST_CASE("a fruit amount is zero only when every component is zero [ui][auction_model]")
{
    CHECK(AuctionAmount::FromFruits(FruitBasket{}).IsZero());
    CHECK_FALSE(AuctionAmount::FromFruits(FruitBasket{ .Energy = 1 }).IsZero());
}

TEST_CASE("only the Fruits currency mode is a fruit currency [ui][auction_model]")
{
    CHECK(IsFruitCurrency(AuctionCurrencyMode::Fruits));
    CHECK_FALSE(IsFruitCurrency(AuctionCurrencyMode::Zen));
    CHECK_FALSE(IsFruitCurrency(AuctionCurrencyMode::Chaos));
}

TEST_CASE("a countdown of zero or negative seconds reads as ended [ui][auction_model]")
{
    CHECK(FormatAuctionCountdown(std::chrono::seconds(0)) == L"Ended");
    CHECK(FormatAuctionCountdown(std::chrono::seconds(-5)) == L"Ended");
}

TEST_CASE("a countdown under a minute shows whole seconds [ui][auction_model]")
{
    CHECK(FormatAuctionCountdown(std::chrono::seconds(45)) == L"45s");
    CHECK(FormatAuctionCountdown(std::chrono::seconds(1)) == L"1s");
}

TEST_CASE("a countdown under an hour shows minutes and seconds [ui][auction_model]")
{
    CHECK(FormatAuctionCountdown(std::chrono::seconds(125)) == L"2m 5s");
}

TEST_CASE("a countdown under a day shows hours and minutes [ui][auction_model]")
{
    CHECK(FormatAuctionCountdown(std::chrono::seconds(3661)) == L"1h 1m");
}

TEST_CASE("a countdown of a day or more shows days and hours [ui][auction_model]")
{
    CHECK(FormatAuctionCountdown(std::chrono::seconds(90000)) == L"1d 1h");
}

TEST_CASE("successive request ids strictly increase [ui][auction_model]")
{
    const auto first = NextAuctionRequestId();
    const auto second = NextAuctionRequestId();
    const auto third = NextAuctionRequestId();

    CHECK(second > first);
    CHECK(third > second);
}

TEST_CASE("two generated operation ids are not the same [ui][auction_model]")
{
    const auto first = GenerateAuctionOperationId();
    const auto second = GenerateAuctionOperationId();

    CHECK_FALSE(first == second);
}

TEST_CASE("an unsynced server clock estimates zero [ui][auction_model]")
{
    const AuctionServerClock clock;

    CHECK_FALSE(clock.HasSynced());
    CHECK(clock.EstimatedServerTime(12345) == 0);
}

TEST_CASE("a synced server clock estimates the exact server time at the moment of sync [ui][auction_model]")
{
    AuctionServerClock clock;
    clock.Sync(1000, 50000);

    CHECK(clock.HasSynced());
    CHECK(clock.EstimatedServerTime(50000) == 1000);
}

TEST_CASE("a synced server clock advances its estimate with local ticks [ui][auction_model]")
{
    AuctionServerClock clock;
    clock.Sync(1000, 50000);

    CHECK(clock.EstimatedServerTime(52500) == 1002);
}

TEST_CASE("resyncing replaces the previous server clock reading [ui][auction_model]")
{
    AuctionServerClock clock;
    clock.Sync(1000, 50000);
    clock.Sync(5000, 90000);

    CHECK(clock.EstimatedServerTime(91000) == 5001);
}

TEST_CASE("a server clock's estimate survives a local tick-count wraparound [ui][auction_model]")
{
    AuctionServerClock clock;
    clock.Sync(1000, 0xFFFFFFF0u);

    // 26ms after the sync point, wrapped around past the uint32_t tick-count maximum.
    CHECK(clock.EstimatedServerTime(10u) == 1000);
}

TEST_CASE("the minimum next bid with no existing bids is the current (starting) price [ui][auction_model]")
{
    const auto current = AuctionAmount::FromScalar(500);
    const auto minimum = ComputeMinimumNextBid(AuctionCurrencyMode::Zen, current, 0);

    CHECK_FALSE(minimum.IsFruitBasket());
    CHECK(minimum.Scalar() == 500);
}

TEST_CASE("a later Zen bid adds at least 1000 units when 5 percent is smaller [ui][auction_model]")
{
    const auto current = AuctionAmount::FromScalar(100);
    const auto minimum = ComputeMinimumNextBid(AuctionCurrencyMode::Zen, current, 1);

    CHECK(minimum.Scalar() == 1100);
}

TEST_CASE("a later Zen bid adds 5 percent when that exceeds the 1000-unit floor [ui][auction_model]")
{
    const auto current = AuctionAmount::FromScalar(100000);
    const auto minimum = ComputeMinimumNextBid(AuctionCurrencyMode::Zen, current, 1);

    CHECK(minimum.Scalar() == 105000);
}

TEST_CASE("a later jewel bid adds at least 1 unit [ui][auction_model]")
{
    const auto current = AuctionAmount::FromScalar(10);
    const auto minimum = ComputeMinimumNextBid(AuctionCurrencyMode::Chaos, current, 1);

    CHECK(minimum.Scalar() == 11);
}

TEST_CASE("a later fruit bid increases only the nonzero components, each by at least 5 percent [ui][auction_model]")
{
    const auto current = AuctionAmount::FromFruits(FruitBasket{ .Strength = 100, .Agility = 0, .Command = 50 });
    const auto minimum = ComputeMinimumNextBid(AuctionCurrencyMode::Fruits, current, 1);

    CHECK(minimum.IsFruitBasket());
    CHECK(minimum.Fruits() == FruitBasket{ .Strength = 105, .Agility = 0, .Command = 53 });
}

TEST_CASE("the first fruit bid is the starting basket unchanged [ui][auction_model]")
{
    const FruitBasket basket{ .Vitality = 7, .Energy = 3 };
    const auto current = AuctionAmount::FromFruits(basket);
    const auto minimum = ComputeMinimumNextBid(AuctionCurrencyMode::Fruits, current, 0);

    CHECK(minimum.Fruits() == basket);
}

TEST_CASE("auction fee preview rounds scalar fees upward with a one-unit minimum [ui][auction_model]")
{
    CHECK(ComputeAuctionFee(AuctionAmount::FromScalar(1), 100).Scalar() == 1);
    CHECK(ComputeAuctionFee(AuctionAmount::FromScalar(100), 100).Scalar() == 1);
    CHECK(ComputeAuctionFee(AuctionAmount::FromScalar(101), 100).Scalar() == 2);
    CHECK(ComputeAuctionFee(AuctionAmount::FromScalar(0), 100).Scalar() == 0);
}

TEST_CASE("auction fee preview rounds every nonzero fruit component independently [ui][auction_model]")
{
    const auto price = AuctionAmount::FromFruits(FruitBasket{
        .Strength = 1,
        .Agility = 100,
        .Vitality = 101,
        .Energy = 0,
        .Command = 999,
    });

    const auto fee = ComputeAuctionFee(price, 100);

    CHECK(fee.IsFruitBasket());
    CHECK(fee.Fruits() == FruitBasket{ .Strength = 1, .Agility = 1, .Vitality = 2, .Energy = 0, .Command = 10 });
}

TEST_CASE("browse scrolling exposes every row beyond the visible viewport [ui][auction_model]")
{
    CHECK(MaximumBrowseScrollOffset(25, 9) == 16);
    CHECK(MaximumBrowseScrollOffset(10, 9) == 1);
}

TEST_CASE("browse scrolling stays at the top when all rows fit [ui][auction_model]")
{
    CHECK(MaximumBrowseScrollOffset(9, 9) == 0);
    CHECK(MaximumBrowseScrollOffset(3, 9) == 0);
    CHECK(MaximumBrowseScrollOffset(0, 9) == 0);
}

TEST_CASE("clicking the price header toggles ascending and descending server sorts [ui][auction_model]")
{
    CHECK(NextBrowseSort(AuctionBrowseSortColumn::Price, AuctionSort::EndingSoonest) == AuctionSort::PriceAscending);
    CHECK(NextBrowseSort(AuctionBrowseSortColumn::Price, AuctionSort::PriceAscending) == AuctionSort::PriceDescending);
    CHECK(NextBrowseSort(AuctionBrowseSortColumn::Price, AuctionSort::PriceDescending) == AuctionSort::PriceAscending);
}

TEST_CASE("clicking time left restores the server's ending-soonest sort [ui][auction_model]")
{
    CHECK(NextBrowseSort(AuctionBrowseSortColumn::TimeLeft, AuctionSort::PriceAscending) == AuctionSort::EndingSoonest);
    CHECK(NextBrowseSort(AuctionBrowseSortColumn::TimeLeft, AuctionSort::Newest) == AuctionSort::EndingSoonest);
}

TEST_CASE("fruit browsing falls back from unsupported scalar price sorting [ui][auction_model]")
{
    CHECK(NormalizeBrowseSort(AuctionCurrencyMode::Fruits, AuctionSort::PriceAscending) == AuctionSort::EndingSoonest);
    CHECK(NormalizeBrowseSort(AuctionCurrencyMode::Fruits, AuctionSort::PriceDescending) == AuctionSort::EndingSoonest);
    CHECK(NormalizeBrowseSort(AuctionCurrencyMode::Chaos, AuctionSort::PriceAscending) == AuctionSort::PriceAscending);
}

TEST_CASE("auction search names respect the fixed UTF-8 byte capacity [ui][auction_model]")
{
    CHECK(ClampUtf8ToByteCapacity(std::string(33, 'a'), 32) == std::string(32, 'a'));
    CHECK(ClampUtf8ToByteCapacity(std::string(29, 'a') + "\xE2\x82\xAC", 32)
        == std::string(29, 'a') + "\xE2\x82\xAC");
}

TEST_CASE("auction search truncation never splits a UTF-8 character [ui][auction_model]")
{
    CHECK(ClampUtf8ToByteCapacity(std::string(30, 'a') + "\xE2\x82\xAC", 32) == std::string(30, 'a'));
}

TEST_CASE("my listings status combo maps all and concrete statuses to the wire [ui][auction_model]")
{
    CHECK(EncodeListingStatusFilter(0) == 0xFF);
    CHECK(EncodeListingStatusFilter(1) == static_cast<uint8_t>(AuctionListingStatus::Active));
    CHECK(EncodeListingStatusFilter(5) == static_cast<uint8_t>(AuctionListingStatus::AdminRemoved));
}

TEST_CASE("only an active owned listing without bids may be cancelled [ui][auction_model]")
{
    CHECK(CanCancelOwnedListing(AuctionListingStatus::Active, 0));
    CHECK_FALSE(CanCancelOwnedListing(AuctionListingStatus::Active, 1));
    CHECK_FALSE(CanCancelOwnedListing(AuctionListingStatus::Sold, 0));
    CHECK_FALSE(CanCancelOwnedListing(AuctionListingStatus::Expired, 0));
    CHECK_FALSE(CanCancelOwnedListing(AuctionListingStatus::Cancelled, 0));
    CHECK_FALSE(CanCancelOwnedListing(AuctionListingStatus::AdminRemoved, 0));
}

TEST_CASE("mailbox kind combo maps all and concrete collection kinds to the wire [ui][auction_model]")
{
    CHECK(EncodeCollectionKindFilter(0) == 0xFF);
    CHECK(EncodeCollectionKindFilter(1) == static_cast<uint8_t>(AuctionCollectionKind::PurchasedItem));
    CHECK(EncodeCollectionKindFilter(6) == static_cast<uint8_t>(AuctionCollectionKind::AdminRefund));
}

TEST_CASE("partial mailbox claims stay within the remaining obligation [ui][auction_model]")
{
    const auto jewels = AuctionAmount::FromScalar(25);
    CHECK(IsValidCollectionClaim(AuctionCurrencyMode::Bless, jewels, AuctionAmount::FromScalar(1)));
    CHECK(IsValidCollectionClaim(AuctionCurrencyMode::Bless, jewels, AuctionAmount::FromScalar(25)));
    CHECK_FALSE(IsValidCollectionClaim(AuctionCurrencyMode::Bless, jewels, AuctionAmount::FromScalar(0)));
    CHECK_FALSE(IsValidCollectionClaim(AuctionCurrencyMode::Bless, jewels, AuctionAmount::FromScalar(26)));

    const auto remaining = AuctionAmount::FromFruits(FruitBasket{ 3, 0, 4, 2, 0 });
    CHECK(IsValidCollectionClaim(AuctionCurrencyMode::Fruits, remaining,
        AuctionAmount::FromFruits(FruitBasket{ 1, 0, 0, 2, 0 })));
    CHECK_FALSE(IsValidCollectionClaim(AuctionCurrencyMode::Fruits, remaining,
        AuctionAmount::FromFruits(FruitBasket{})));
    CHECK_FALSE(IsValidCollectionClaim(AuctionCurrencyMode::Fruits, remaining,
        AuctionAmount::FromFruits(FruitBasket{ 4, 0, 0, 0, 0 })));
    CHECK_FALSE(IsValidCollectionClaim(AuctionCurrencyMode::Fruits, remaining,
        AuctionAmount::FromFruits(FruitBasket{ 1, 1, 0, 0, 0 })));
}

TEST_CASE("wallet count candidates list the exact digits first, then shorter compact forms [ui][auction_model]")
{
    CHECK(WalletCountCandidates(0) == std::vector<std::wstring>{ L"0" });
    CHECK(WalletCountCandidates(999) == std::vector<std::wstring>{ L"999" });
    CHECK(WalletCountCandidates(1000) == std::vector<std::wstring>{ L"1000", L"1k" });
    CHECK(WalletCountCandidates(12345) == std::vector<std::wstring>{ L"12345", L"12.3k", L"12k" });
    CHECK(WalletCountCandidates(1500000) == std::vector<std::wstring>{ L"1500000", L"1.5M", L"1M" });
    CHECK(WalletCountCandidates(2000000000) == std::vector<std::wstring>{ L"2000000000", L"2B" });
}

TEST_CASE("compact wallet counts truncate instead of rounding up to the next unit [ui][auction_model]")
{
    // 999,999 must never read as "1000k" or "1M": that would claim more than the player has.
    CHECK(WalletCountCandidates(999999) == std::vector<std::wstring>{ L"999999", L"999.9k", L"999k" });
}

TEST_CASE("a wallet count keeps its exact digits when they fit the available width [ui][auction_model]")
{
    // Every glyph is five pixels wide except the decimal point, which is narrow like a real font's.
    const auto measure = [](const std::wstring& text)
    {
        int width = 0;
        for (const wchar_t glyph : text)
        {
            width += glyph == L'.' ? 2 : 5;
        }
        return width;
    };

    CHECK(FormatWalletCountToFit(8, 30, measure) == L"8");
    CHECK(FormatWalletCountToFit(1234, 20, measure) == L"1234");
}

TEST_CASE("a wallet count falls back to the most precise compact form that fits [ui][auction_model]")
{
    // Every glyph is five pixels wide except the decimal point, which is narrow like a real font's.
    const auto measure = [](const std::wstring& text)
    {
        int width = 0;
        for (const wchar_t glyph : text)
        {
            width += glyph == L'.' ? 2 : 5;
        }
        return width;
    };

    CHECK(FormatWalletCountToFit(12345, 22, measure) == L"12.3k");
    CHECK(FormatWalletCountToFit(12345, 20, measure) == L"12k");
}

TEST_CASE("a wallet count that fits nowhere still returns its shortest form [ui][auction_model]")
{
    // Every glyph is five pixels wide except the decimal point, which is narrow like a real font's.
    const auto measure = [](const std::wstring& text)
    {
        int width = 0;
        for (const wchar_t glyph : text)
        {
            width += glyph == L'.' ? 2 : 5;
        }
        return width;
    };

    CHECK(FormatWalletCountToFit(123456789, 5, measure) == L"123M");
}

TEST_CASE("wallet rows hold at most the given number of columns [ui][auction_model]")
{
    CHECK(WalletRowCount(0, 4) == 0);
    CHECK(WalletRowCount(1, 4) == 1);
    CHECK(WalletRowCount(4, 4) == 1);
    CHECK(WalletRowCount(7, 4) == 2);
    CHECK(WalletRowCount(5, 5) == 1);
}

TEST_CASE("a full wallet row splits the panel width evenly [ui][auction_model]")
{
    const auto first = LayoutWalletCell(0, 7, 4, 114);
    const auto last = LayoutWalletCell(3, 7, 4, 114);

    CHECK(first.row == 0);
    CHECK(first.width == 28);
    CHECK(last.row == 0);
    CHECK(last.x - first.x == 3 * 28);
}

TEST_CASE("a short last wallet row is centred under the full rows [ui][auction_model]")
{
    const auto topLeft = LayoutWalletCell(0, 7, 4, 114);
    const auto bottomLeft = LayoutWalletCell(4, 7, 4, 114);
    const auto bottomRight = LayoutWalletCell(6, 7, 4, 114);

    CHECK(bottomLeft.row == 1);
    CHECK(bottomLeft.x > topLeft.x);
    // Equal margin each side of the three cells.
    CHECK(bottomLeft.x + 3 * bottomLeft.width + bottomLeft.x == 114);
    CHECK(bottomRight.x == bottomLeft.x + 2 * bottomLeft.width);
}

TEST_CASE("fewer cells than columns widen to share the panel [ui][auction_model]")
{
    const auto only = LayoutWalletCell(0, 1, 4, 114);
    const auto second = LayoutWalletCell(1, 2, 4, 114);

    CHECK(only.width == 114);
    CHECK(only.x == 0);
    CHECK(second.width == 57);
    CHECK(second.x == 57);
}

TEST_CASE("a zero price has no components [ui][auction_model]")
{
    CHECK(PriceComponentsOf(AuctionCurrencyMode::Zen, AuctionAmount::FromScalar(0)).empty());
    CHECK(PriceComponentsOf(AuctionCurrencyMode::Fruits, AuctionAmount::FromFruits(FruitBasket{})).empty());
}

TEST_CASE("a scalar price is one component in its own currency [ui][auction_model]")
{
    const auto zen = PriceComponentsOf(AuctionCurrencyMode::Zen, AuctionAmount::FromScalar(137500));
    REQUIRE(zen.size() == 1);
    CHECK(zen[0].kind == PriceKind::Zen);
    CHECK(zen[0].amount == 137500);

    const auto bless = PriceComponentsOf(AuctionCurrencyMode::Bless, AuctionAmount::FromScalar(4));
    REQUIRE(bless.size() == 1);
    CHECK(bless[0].kind == PriceKind::Bless);
    CHECK(bless[0].amount == 4);

    const auto harmony = PriceComponentsOf(AuctionCurrencyMode::Harmony, AuctionAmount::FromScalar(9));
    REQUIRE(harmony.size() == 1);
    CHECK(harmony[0].kind == PriceKind::Harmony);
}

TEST_CASE("a fruit price lists each nonzero fruit as its own component, in fruit order [ui][auction_model]")
{
    const auto components = PriceComponentsOf(AuctionCurrencyMode::Fruits,
        AuctionAmount::FromFruits(FruitBasket{ .Strength = 500, .Energy = 12, .Command = 3 }));

    REQUIRE(components.size() == 3);
    CHECK(components[0].kind == PriceKind::StrengthFruit);
    CHECK(components[0].amount == 500);
    CHECK(components[1].kind == PriceKind::EnergyFruit);
    CHECK(components[1].amount == 12);
    CHECK(components[2].kind == PriceKind::CommandFruit);
    CHECK(components[2].amount == 3);
}

TEST_CASE("packing no price chips succeeds with no placements [ui][auction_model]")
{
    const auto placements = PackPriceChips({}, 100, 4, 2);

    REQUIRE(placements.has_value());
    CHECK(placements->empty());
}

TEST_CASE("a single price chip is centred in its line [ui][auction_model]")
{
    const auto placements = PackPriceChips({ 30 }, 100, 4, 2);

    REQUIRE(placements.has_value());
    REQUIRE(placements->size() == 1);
    CHECK((*placements)[0].line == 0);
    CHECK((*placements)[0].x == 35);
}

TEST_CASE("price chips share a line until the next one would not fit [ui][auction_model]")
{
    const auto placements = PackPriceChips({ 30, 30, 30 }, 100, 4, 2);

    REQUIRE(placements.has_value());
    REQUIRE(placements->size() == 3);
    CHECK((*placements)[0].line == 0);
    CHECK((*placements)[2].line == 0);
    CHECK((*placements)[1].x - (*placements)[0].x == 34);
    // 30+4+30+4+30 = 98 wide, centred in 100.
    CHECK((*placements)[0].x == 1);
}

TEST_CASE("five price chips wrap to a second line that is centred on its own [ui][auction_model]")
{
    const auto placements = PackPriceChips({ 30, 30, 30, 30, 30 }, 100, 4, 2);

    REQUIRE(placements.has_value());
    REQUIRE(placements->size() == 5);
    CHECK((*placements)[3].line == 1);
    CHECK((*placements)[4].line == 1);
    // 30+4+30 = 64 wide, centred in 100.
    CHECK((*placements)[3].x == 18);
    CHECK((*placements)[4].x == 52);
}

TEST_CASE("price chips that cannot be packed report failure rather than overflowing [ui][auction_model]")
{
    CHECK_FALSE(PackPriceChips({ 120 }, 100, 4, 2).has_value());
    CHECK_FALSE(PackPriceChips({ 60, 60, 60, 60, 60 }, 100, 4, 2).has_value());
}

TEST_CASE("a window beside another keeps its place when it fits and slides left only as far as it must [ui][auction_model]")
{
    // Fits: right edge 100 + 600 = 700 is left of the limit 900.
    CHECK(SideBySideLeft(100, 600, 900, -500) == 100);
    // Exactly touching the limit still fits.
    CHECK(SideBySideLeft(300, 600, 900, -500) == 300);
    // Overlaps by 50, so it slides left by 50.
    CHECK(SideBySideLeft(350, 600, 900, -500) == 300);
    // Sliding is bounded by the left edge of the screen.
    CHECK(SideBySideLeft(350, 600, 900, 320) == 320);
    // The screen edge wins even when the window cannot fit at all.
    CHECK(SideBySideLeft(0, 600, 500, 10) == 10);
}
