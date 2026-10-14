#include <doctest.h>

#include "UI/NewUI/AuctionHouse/AuctionModel.h"

using namespace AuctionHouse;

namespace
{
    SellDraft AuctionDraft(PriceKind kind, int64_t quantity, int64_t buyout = 0)
    {
        SellDraft draft;
        draft.mode = SellMode::Auction;
        draft.rows = { SellPriceRow{ kind, quantity, buyout } };
        return draft;
    }

    SellDraft MixedAuctionDraft(std::vector<SellPriceRow> rows)
    {
        SellDraft draft;
        draft.mode = SellMode::Auction;
        draft.rows = std::move(rows);
        return draft;
    }

    SellDraft BuyNowDraft(std::vector<SellPriceRow> rows)
    {
        SellDraft draft;
        draft.mode = SellMode::BuyNow;
        draft.rows = std::move(rows);
        return draft;
    }
}

TEST_CASE("an auction with one priced component is a valid draft [ui][auction_sell]")
{
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Bless, 5), true) == SellDraftIssue::None);
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Zen, 250000, 400000), true) == SellDraftIssue::None);
}

TEST_CASE("an auction without a price or with a zero price is rejected [ui][auction_sell]")
{
    SellDraft empty;
    empty.mode = SellMode::Auction;

    CHECK(ValidateSellDraft(empty, true) == SellDraftIssue::NoPrice);
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Bless, 0), true) == SellDraftIssue::ZeroQuantity);
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Bless, -3), true) == SellDraftIssue::ZeroQuantity);
}

TEST_CASE("an auction buyout is optional and not below the starting price [ui][auction_sell]")
{
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Soul, 10, 0), true) == SellDraftIssue::None);
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Soul, 10, 10), true) == SellDraftIssue::None);
    CHECK(ValidateSellDraft(AuctionDraft(PriceKind::Soul, 10, 9), true) == SellDraftIssue::BuyoutBelowStart);
}

TEST_CASE("an auction in several currencies needs a server that takes mixed prices [ui][auction_sell]")
{
    const auto draft = MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 10 }, SellPriceRow{ PriceKind::Bless, 2 } });

    CHECK(ValidateSellDraft(draft, true) == SellDraftIssue::None);
    CHECK(ValidateSellDraft(draft, false) == SellDraftIssue::MixedUnavailable);
}

TEST_CASE("an auction mixes up to five distinct kinds with positive amounts [ui][auction_sell]")
{
    auto five = MixedAuctionDraft({ SellPriceRow{ PriceKind::Zen, 5000 }, SellPriceRow{ PriceKind::Chaos, 1 },
        SellPriceRow{ PriceKind::Bless, 1 }, SellPriceRow{ PriceKind::Soul, 1 }, SellPriceRow{ PriceKind::Life, 1 } });
    auto six = five;
    six.rows.push_back(SellPriceRow{ PriceKind::Creation, 1 });
    auto duplicate = MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 1 }, SellPriceRow{ PriceKind::Soul, 2 } });
    auto zero = MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 1 }, SellPriceRow{ PriceKind::Bless, 0 } });

    CHECK(ValidateSellDraft(five, true) == SellDraftIssue::None);
    CHECK(ValidateSellDraft(six, true) == SellDraftIssue::TooManyComponents);
    CHECK(ValidateSellDraft(duplicate, true) == SellDraftIssue::DuplicateKind);
    CHECK(ValidateSellDraft(zero, true) == SellDraftIssue::ZeroQuantity);
}

TEST_CASE("a mixed auction names a buyout in every currency or in none [ui][auction_sell]")
{
    CHECK(ValidateSellDraft(MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 10, 12 }, SellPriceRow{ PriceKind::Bless, 2, 2 } }), true)
        == SellDraftIssue::None);
    CHECK(ValidateSellDraft(MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 10, 12 }, SellPriceRow{ PriceKind::Bless, 2, 0 } }), true)
        == SellDraftIssue::BuyoutIncomplete);
    CHECK(ValidateSellDraft(MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 10, 12 }, SellPriceRow{ PriceKind::Bless, 2, 1 } }), true)
        == SellDraftIssue::BuyoutBelowStart);
}

TEST_CASE("buy now is refused while the server cannot take mixed prices [ui][auction_sell]")
{
    const auto draft = BuyNowDraft({ SellPriceRow{ PriceKind::Bless, 4 } });

    CHECK(ValidateSellDraft(draft, false) == SellDraftIssue::BuyNowUnavailable);
    CHECK(ValidateSellDraft(draft, true) == SellDraftIssue::None);
}

TEST_CASE("a buy now price mixes up to five kinds [ui][auction_sell]")
{
    const auto five = BuyNowDraft({
        SellPriceRow{ PriceKind::Zen, 500000 }, SellPriceRow{ PriceKind::Chaos, 10 }, SellPriceRow{ PriceKind::Bless, 4 },
        SellPriceRow{ PriceKind::StrengthFruit, 500 }, SellPriceRow{ PriceKind::Life, 103 } });
    auto six = five;
    six.rows.push_back(SellPriceRow{ PriceKind::Creation, 11 });

    CHECK(ValidateSellDraft(five, true) == SellDraftIssue::None);
    CHECK(ValidateSellDraft(six, true) == SellDraftIssue::TooManyComponents);
    CHECK(ValidateSellDraft(six, true, 6) == SellDraftIssue::None);
}

TEST_CASE("a buy now price needs distinct kinds and positive quantities [ui][auction_sell]")
{
    CHECK(ValidateSellDraft(BuyNowDraft({}), true) == SellDraftIssue::NoPrice);
    CHECK(ValidateSellDraft(BuyNowDraft({ SellPriceRow{ PriceKind::Bless, 1 }, SellPriceRow{ PriceKind::Bless, 2 } }), true)
        == SellDraftIssue::DuplicateKind);
    CHECK(ValidateSellDraft(BuyNowDraft({ SellPriceRow{ PriceKind::Bless, 1 }, SellPriceRow{ PriceKind::Soul, 0 } }), true)
        == SellDraftIssue::ZeroQuantity);
}

TEST_CASE("an auction in Zen or a jewel maps to the scalar create fields [ui][auction_sell]")
{
    const auto fields = ToCreateFields(AuctionDraft(PriceKind::Bless, 5, 9));

    REQUIRE(fields.has_value());
    CHECK(fields->currencyMode == AuctionCurrencyMode::Bless);
    CHECK_FALSE(fields->starting.IsFruitBasket());
    CHECK(fields->starting.Scalar() == 5);
    CHECK(fields->buyout.Scalar() == 9);

    const auto zen = ToCreateFields(AuctionDraft(PriceKind::Zen, 250000));
    REQUIRE(zen.has_value());
    CHECK(zen->currencyMode == AuctionCurrencyMode::Zen);
    CHECK(zen->buyout.IsZero());
}

TEST_CASE("an auction in a stat fruit maps to a fruit basket with that one component [ui][auction_sell]")
{
    const auto fields = ToCreateFields(AuctionDraft(PriceKind::EnergyFruit, 20, 30));

    REQUIRE(fields.has_value());
    CHECK(fields->currencyMode == AuctionCurrencyMode::Fruits);
    CHECK(fields->starting.IsFruitBasket());
    CHECK(fields->starting.Fruits() == FruitBasket{ .Energy = 20 });
    CHECK(fields->buyout.Fruits() == FruitBasket{ .Energy = 30 });
}

TEST_CASE("a buy now draft becomes a mixed create with only a buyout basket [ui][auction_sell]")
{
    const auto fields = ToCreateFields(BuyNowDraft({ SellPriceRow{ PriceKind::Bless, 4 }, SellPriceRow{ PriceKind::Zen, 50000 },
        SellPriceRow{ PriceKind::CommandFruit, 7 } }));

    REQUIRE(fields.has_value());
    CHECK(fields->currencyMode == AuctionCurrencyMode::Mixed);
    CHECK(fields->startingBasket == BasketQuantities{});
    CHECK(fields->buyoutBasket == BasketQuantities{ 50000, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7 });
}

TEST_CASE("an auction in several currencies becomes a mixed create with both baskets [ui][auction_sell]")
{
    const auto fields = ToCreateFields(MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 3, 5 }, SellPriceRow{ PriceKind::Zen, 50000, 80000 } }));

    REQUIRE(fields.has_value());
    CHECK(fields->currencyMode == AuctionCurrencyMode::Mixed);
    CHECK(fields->startingBasket == BasketQuantities{ 50000, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
    CHECK(fields->buyoutBasket == BasketQuantities{ 80000, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0 });

    const auto withoutBuyout = ToCreateFields(MixedAuctionDraft({ SellPriceRow{ PriceKind::Soul, 3 }, SellPriceRow{ PriceKind::Zen, 50000 } }));
    REQUIRE(withoutBuyout.has_value());
    CHECK(withoutBuyout->buyoutBasket == BasketQuantities{});
}

TEST_CASE("an empty draft has no create fields [ui][auction_sell]")
{
    CHECK_FALSE(ToCreateFields(SellDraft{}).has_value());
}

TEST_CASE("the fee for a sell draft is the listing fee on its starting price [ui][auction_sell]")
{
    // 1% (100 basis points) of 250,000 Zen is 2,500; of 5 Bless it rounds up to the one-unit minimum.
    const auto zen = ToCreateFields(AuctionDraft(PriceKind::Zen, 250000));
    const auto bless = ToCreateFields(AuctionDraft(PriceKind::Bless, 5));

    REQUIRE(zen.has_value());
    REQUIRE(bless.has_value());
    CHECK(ComputeAuctionFee(zen->starting, 100).Scalar() == 2500);
    CHECK(ComputeAuctionFee(bless->starting, 100).Scalar() == 1);
}

TEST_CASE("every create result has a message for the seller [ui][auction_sell]")
{
    for (uint32_t value = 0; value <= static_cast<uint32_t>(AuctionResult::ServiceUnavailable); ++value)
    {
        const wchar_t* text = DescribeCreateResult(static_cast<AuctionResult>(value));
        REQUIRE(text != nullptr);
        CHECK(text[0] != L'\0');
    }
}

TEST_CASE("the common create failures say what to do about them [ui][auction_sell]")
{
    CHECK(std::wstring(DescribeCreateResult(AuctionResult::Success)) == L"Listing created.");
    CHECK(std::wstring(DescribeCreateResult(AuctionResult::InsufficientCurrency)).find(L"fee") != std::wstring::npos);
    CHECK(std::wstring(DescribeCreateResult(AuctionResult::ItemNotEligible)).find(L"cannot be sold") != std::wstring::npos);
    CHECK(std::wstring(DescribeCreateResult(AuctionResult::ListingLimitReached)).find(L"limit") != std::wstring::npos);
}

TEST_CASE("every bid, buyout and cancel result has a message [ui][auction_sell]")
{
    for (const auto type : { AuctionOperationType::Bid, AuctionOperationType::Buyout, AuctionOperationType::Cancel })
    {
        for (uint32_t value = 0; value <= static_cast<uint32_t>(AuctionResult::ServiceUnavailable); ++value)
        {
            const wchar_t* text = DescribeOperationResult(type, static_cast<AuctionResult>(value));
            REQUIRE(text != nullptr);
            CHECK(text[0] != L'\0');
        }
    }
}

TEST_CASE("bid failures say why the bid did not go through [ui][auction_sell]")
{
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Bid, AuctionResult::Success)) == L"Bid placed.");
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Bid, AuctionResult::BidTooLow)).find(L"too low") != std::wstring::npos);
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Bid, AuctionResult::AlreadyHighestBidder)).find(L"highest") != std::wstring::npos);
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Bid, AuctionResult::SelfTrade)).find(L"own") != std::wstring::npos);
}

TEST_CASE("a purchase and a cancellation point the player to the Mailbox [ui][auction_sell]")
{
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Buyout, AuctionResult::Success)).find(L"Mailbox") != std::wstring::npos);
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Cancel, AuctionResult::Success)).find(L"Mailbox") != std::wstring::npos);
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Cancel, AuctionResult::CannotCancelWithBid)).find(L"bids") != std::wstring::npos);
}

TEST_CASE("a create result is described the same through the general function [ui][auction_sell]")
{
    CHECK(std::wstring(DescribeOperationResult(AuctionOperationType::Create, AuctionResult::InsufficientCurrency))
        == std::wstring(DescribeCreateResult(AuctionResult::InsufficientCurrency)));
}

TEST_CASE("a basket amount lists its priced kinds in kind order [ui][auction_sell]")
{
    const auto amount = AuctionAmount::FromBasket(BasketQuantities{ 50000, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 12 });

    const auto components = PriceComponentsOf(AuctionCurrencyMode::Mixed, amount);

    REQUIRE(components.size() == 3);
    CHECK(components[0].kind == PriceKind::Zen);
    CHECK(components[0].amount == 50000);
    CHECK(components[1].kind == PriceKind::Soul);
    CHECK(components[2].kind == PriceKind::CommandFruit);
    CHECK_FALSE(amount.IsZero());
    CHECK(AuctionAmount::FromBasket(BasketQuantities{}).IsZero());
}

TEST_CASE("the first bid on a mixed listing must reach the starting price in every currency [ui][auction_sell]")
{
    const BasketQuantities starting{ 50000, 0, 0, 3 };
    const BasketQuantities none{};

    CHECK(IsMixedBidAllowed(starting, none, false, BasketQuantities{ 50000, 0, 0, 3 }));
    CHECK(IsMixedBidAllowed(starting, none, false, BasketQuantities{ 60000, 0, 1, 4 }));
    CHECK_FALSE(IsMixedBidAllowed(starting, none, false, BasketQuantities{ 50000, 0, 0, 2 }));
    CHECK_FALSE(IsMixedBidAllowed(starting, none, false, BasketQuantities{ 0, 0, 0, 9 }));
    CHECK_FALSE(IsMixedBidAllowed(starting, none, false, none));
}

TEST_CASE("a later bid keeps every currency of the current bid and raises one [ui][auction_sell]")
{
    const BasketQuantities starting{ 50000, 0, 0, 3 };
    const BasketQuantities current{ 50000, 0, 1, 4 };

    CHECK(IsMixedBidAllowed(starting, current, true, BasketQuantities{ 50000, 0, 1, 5 }));
    CHECK(IsMixedBidAllowed(starting, current, true, BasketQuantities{ 50000, 1, 1, 4 }));
    CHECK_FALSE(IsMixedBidAllowed(starting, current, true, current));
    CHECK_FALSE(IsMixedBidAllowed(starting, current, true, BasketQuantities{ 90000, 0, 0, 9 }));
}

TEST_CASE("a bid names at most five currencies and no more than the wire can carry [ui][auction_sell]")
{
    const BasketQuantities starting{ 1, 1, 1, 1, 1 };
    CHECK_FALSE(IsMixedBidAllowed(starting, BasketQuantities{}, false, BasketQuantities{ 1, 1, 1, 1, 1, 1 }));
    CHECK_FALSE(IsMixedBidAllowed(BasketQuantities{ 1 }, BasketQuantities{}, false, BasketQuantities{ 5000000000LL }));
}

TEST_CASE("the suggested mixed bid is the starting price, then the current bid plus one of its first currency [ui][auction_sell]")
{
    CHECK(SuggestedMixedBid(BasketQuantities{ 50000, 0, 0, 3 }, false) == BasketQuantities{ 50000, 0, 0, 3 });
    CHECK(SuggestedMixedBid(BasketQuantities{ 0, 0, 1, 4 }, true) == BasketQuantities{ 0, 0, 2, 4 });
}

TEST_CASE("the minimum next bid on a mixed listing follows the same rule [ui][auction_sell]")
{
    const auto current = AuctionAmount::FromBasket(BasketQuantities{ 50000, 0, 0, 3 });

    const auto first = ComputeMinimumNextBid(AuctionCurrencyMode::Mixed, current, 0);
    const auto later = ComputeMinimumNextBid(AuctionCurrencyMode::Mixed, current, 2);

    CHECK(first.Basket() == BasketQuantities{ 50000, 0, 0, 3 });
    CHECK(later.Basket() == BasketQuantities{ 50001, 0, 0, 3 });
}
