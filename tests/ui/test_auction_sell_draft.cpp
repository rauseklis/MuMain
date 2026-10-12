#include <doctest.h>

#include "UI/NewUI/AuctionHouse/AuctionModel.h"

using namespace AuctionHouse;

namespace
{
    SellDraft AuctionDraft(PriceKind kind, int64_t quantity, int64_t buyout = 0)
    {
        SellDraft draft;
        draft.mode = SellMode::Auction;
        draft.rows = { SellPriceRow{ kind, quantity } };
        draft.auctionBuyoutQuantity = buyout;
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

TEST_CASE("an auction takes exactly one price row [ui][auction_sell]")
{
    SellDraft draft = AuctionDraft(PriceKind::Soul, 10);
    draft.rows.push_back(SellPriceRow{ PriceKind::Bless, 2 });

    CHECK(ValidateSellDraft(draft, true) == SellDraftIssue::TooManyComponents);
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

TEST_CASE("a buy now draft has no create fields on the current protocol [ui][auction_sell]")
{
    CHECK_FALSE(ToCreateFields(BuyNowDraft({ SellPriceRow{ PriceKind::Bless, 4 } })).has_value());
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
