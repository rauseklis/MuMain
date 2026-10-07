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
