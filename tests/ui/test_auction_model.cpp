#include <doctest.h>

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
