#include <doctest.h>

#include <cstring>
#include <vector>

#include "UI/NewUI/AuctionHouse/AuctionWireResponses.h"

using namespace AuctionHouse;

namespace
{
    void PutU8(std::vector<uint8_t>& buffer, size_t offset, uint8_t value)
    {
        buffer[offset] = value;
    }

    void PutU16(std::vector<uint8_t>& buffer, size_t offset, uint16_t value)
    {
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
    }

    void PutU32(std::vector<uint8_t>& buffer, size_t offset, uint32_t value)
    {
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
    }

    void PutU64(std::vector<uint8_t>& buffer, size_t offset, uint64_t value)
    {
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
    }

    void PutAscii(std::vector<uint8_t>& buffer, size_t offset, const char* text)
    {
        std::memcpy(buffer.data() + offset, text, std::strlen(text));
    }

    // Builds a scalar-currency AuctionOperationResponse packet (currency != Fruits: only the *Scalar fields are set).
    std::vector<uint8_t> ScalarOperationPacket()
    {
        std::vector<uint8_t> packet(AuctionOperationResponse::FullWireLength, 0);
        for (uint8_t i = 0; i < 16; ++i)
        {
            PutU8(packet, 4 + i, i + 1);
        }

        PutU8(packet, 20, static_cast<uint8_t>(AuctionOperationType::Bid));
        PutU8(packet, 21, static_cast<uint8_t>(AuctionResult::Success));
        PutU64(packet, 22, 999001ULL);
        PutU64(packet, 30, 0ULL);
        PutU32(packet, 38, 7U);
        PutU8(packet, 42, static_cast<uint8_t>(AuctionCurrencyMode::Chaos));
        PutU32(packet, 43, 1100U); // fee scalar
        PutU32(packet, 67, 50000U); // spendable scalar
        PutU32(packet, 139, 1700000000U); // server time
        return packet;
    }

    // Builds a Fruits-currency AuctionOperationResponse packet: the five fruit fields are set on each amount instead.
    std::vector<uint8_t> FruitOperationPacket()
    {
        std::vector<uint8_t> packet(AuctionOperationResponse::FullWireLength, 0);
        PutU8(packet, 20, static_cast<uint8_t>(AuctionOperationType::Buyout));
        PutU8(packet, 21, static_cast<uint8_t>(AuctionResult::Success));
        PutU64(packet, 22, 42ULL);
        PutU8(packet, 42, static_cast<uint8_t>(AuctionCurrencyMode::Fruits));
        // Fee: Strength@47, Agility@51, Vitality@55, Energy@59, Command@63.
        PutU32(packet, 47, 1U);
        PutU32(packet, 51, 2U);
        PutU32(packet, 55, 3U);
        PutU32(packet, 59, 4U);
        PutU32(packet, 63, 5U);
        // Claimed: Strength@95.
        PutU32(packet, 95, 10U);
        return packet;
    }
}

TEST_CASE("a scalar operation response reads its identifiers, result and scalar amounts [ui][auction_wire]")
{
    const auto packet = ScalarOperationPacket();

    const auto response = AuctionOperationResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->OperationType == AuctionOperationType::Bid);
    CHECK(response->Result == AuctionResult::Success);
    CHECK(response->ListingId == 999001ULL);
    CHECK(response->CurrencyMode == AuctionCurrencyMode::Chaos);
    CHECK(response->ListingVersion == 7U);
    CHECK_FALSE(response->Fee.IsFruitBasket());
    CHECK(response->Fee.Scalar() == 1100U);
    CHECK(response->Spendable.Scalar() == 50000U);
    CHECK(response->ServerTime == 1700000000U);
    for (uint8_t i = 0; i < 16; ++i)
    {
        CHECK(response->OperationId[i] == i + 1);
    }
}

TEST_CASE("a fruits operation response reads its amounts as fruit baskets [ui][auction_wire]")
{
    const auto packet = FruitOperationPacket();

    const auto response = AuctionOperationResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->CurrencyMode == AuctionCurrencyMode::Fruits);
    CHECK(response->Fee.IsFruitBasket());
    CHECK(response->Fee.Fruits() == FruitBasket{ .Strength = 1, .Agility = 2, .Vitality = 3, .Energy = 4, .Command = 5 });
    CHECK(response->Claimed.Fruits().Strength == 10);
}

TEST_CASE("a truncated operation response fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionOperationResponse::FullWireLength - 1, 0);

    CHECK_FALSE(AuctionOperationResponse::Parse(packet).has_value());
}

TEST_CASE("a notification reads its kind, listing, amount and pending count [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionNotification::FullWireLength, 0);
    PutU8(packet, 4, static_cast<uint8_t>(AuctionNotificationKind::Outbid));
    PutU64(packet, 5, 555ULL);
    PutU8(packet, 13, static_cast<uint8_t>(AuctionCurrencyMode::Zen));
    PutU32(packet, 14, 20000U); // amount scalar
    PutU16(packet, 38, 3U); // pending mailbox count

    const auto notification = AuctionNotification::Parse(packet);

    REQUIRE(notification.has_value());
    CHECK(notification->NotificationKind == AuctionNotificationKind::Outbid);
    CHECK(notification->ListingId == 555ULL);
    CHECK_FALSE(notification->Amount.IsFruitBasket());
    CHECK(notification->Amount.Scalar() == 20000U);
    CHECK(notification->PendingMailboxCount == 3U);
}

TEST_CASE("a truncated notification fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionNotification::FullWireLength - 1, 0);

    CHECK_FALSE(AuctionNotification::Parse(packet).has_value());
}

namespace
{
    // Writes one 95-byte AuctionListingSummary entry at offset into buffer, which must already be at least
    // offset + AuctionListingSummary::WireLength bytes long.
    void PutListingSummary(std::vector<uint8_t>& buffer, size_t offset, uint64_t listingId, const char* sellerName)
    {
        PutU64(buffer, offset + 0, listingId);
        PutU32(buffer, offset + 8, 3U); // version
        PutU8(buffer, offset + 12, static_cast<uint8_t>(AuctionListingStatus::Active));
        PutU8(buffer, offset + 13, static_cast<uint8_t>(AuctionCurrencyMode::Zen));
        PutU8(buffer, offset + 14, static_cast<uint8_t>(AuctionCategory::Weapon));
        PutU32(buffer, offset + 15, 1000U); // current price scalar
        PutU32(buffer, offset + 39, 5000U); // buyout price scalar
        PutU16(buffer, offset + 63, 2U); // bid count
        PutU32(buffer, offset + 65, 1700000000U); // ends at
        PutAscii(buffer, offset + 69, sellerName);
        PutU8(buffer, offset + 79, 15U); // item data length
        PutU8(buffer, offset + 80, 0xAB); // first byte of item data, just to check it round-trips
    }
}

TEST_CASE("a listing summary reads its identifiers, prices, seller and item data [ui][auction_wire]")
{
    std::vector<uint8_t> entry(AuctionListingSummary::WireLength, 0);
    PutListingSummary(entry, 0, 777ULL, "Seller1");

    const auto summary = AuctionListingSummary::Parse(entry);

    REQUIRE(summary.has_value());
    CHECK(summary->ListingId == 777ULL);
    CHECK(summary->Version == 3U);
    CHECK(summary->Status == AuctionListingStatus::Active);
    CHECK(summary->CurrencyMode == AuctionCurrencyMode::Zen);
    CHECK(summary->Category == AuctionCategory::Weapon);
    CHECK(summary->CurrentPrice.Scalar() == 1000U);
    CHECK(summary->BuyoutPrice.Scalar() == 5000U);
    CHECK(summary->BidCount == 2U);
    CHECK(summary->EndsAt == 1700000000U);
    CHECK(summary->SellerName == L"Seller1");
    CHECK(summary->ItemDataLength == 15U);
    CHECK(summary->ItemData[0] == 0xAB);
}

TEST_CASE("a truncated listing summary fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> entry(AuctionListingSummary::WireLength - 1, 0);

    CHECK_FALSE(AuctionListingSummary::Parse(entry).has_value());
}

TEST_CASE("a browse response reads its header and every listing row [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionBrowseResponse::FixedWireLength + 2 * AuctionListingSummary::WireLength, 0);
    PutU32(packet, 5, 4242U); // request id
    PutU8(packet, 9, static_cast<uint8_t>(AuctionResult::Success));
    PutU16(packet, 10, 1U); // page
    PutU16(packet, 12, 3U); // total pages
    PutU32(packet, 14, 25U); // total count
    PutU32(packet, 18, 1700000500U); // server time
    PutU8(packet, 22, 2U); // listing count
    PutListingSummary(packet, 23, 1001ULL, "Alice");
    PutListingSummary(packet, 23 + AuctionListingSummary::WireLength, 1002ULL, "Bob");

    const auto response = AuctionBrowseResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->RequestId == 4242U);
    CHECK(response->Result == AuctionResult::Success);
    CHECK(response->Page == 1U);
    CHECK(response->TotalPages == 3U);
    CHECK(response->TotalCount == 25U);
    CHECK(response->ServerTime == 1700000500U);
    REQUIRE(response->Listings.size() == 2U);
    CHECK(response->Listings[0].ListingId == 1001ULL);
    CHECK(response->Listings[0].SellerName == L"Alice");
    CHECK(response->Listings[1].ListingId == 1002ULL);
    CHECK(response->Listings[1].SellerName == L"Bob");
}

TEST_CASE("a browse response truncated mid-entry fails to parse [ui][auction_wire]")
{
    // Fixed header claims 2 listings, but the buffer only holds one full entry.
    std::vector<uint8_t> packet(AuctionBrowseResponse::FixedWireLength + AuctionListingSummary::WireLength, 0);
    PutU8(packet, 22, 2U);
    PutListingSummary(packet, 23, 1ULL, "Only");

    CHECK_FALSE(AuctionBrowseResponse::Parse(packet).has_value());
}

TEST_CASE("a browse response shorter than its own fixed header fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionBrowseResponse::FixedWireLength - 1, 0);

    CHECK_FALSE(AuctionBrowseResponse::Parse(packet).has_value());
}

TEST_CASE("a detail response reads every field, including seller, bidder, note and item data [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionDetailResponse::FullWireLength, 0);
    PutU32(packet, 5, 99U); // request id
    PutU8(packet, 9, static_cast<uint8_t>(AuctionResult::Success));
    PutU32(packet, 10, 1700000600U); // server time
    PutU64(packet, 14, 555ULL); // listing id
    PutU32(packet, 22, 4U); // version
    PutU8(packet, 26, static_cast<uint8_t>(AuctionListingStatus::Active));
    PutU8(packet, 27, static_cast<uint8_t>(AuctionCurrencyMode::Zen));
    PutU8(packet, 28, static_cast<uint8_t>(AuctionCategory::Armor));
    PutU32(packet, 29, 2000U); // current price scalar
    PutU32(packet, 53, 9000U); // buyout price scalar
    PutU16(packet, 77, 4U); // bid count
    PutU32(packet, 79, 1700000000U); // original ends at
    PutU32(packet, 83, 1700003600U); // ends at
    PutAscii(packet, 87, "Seller1");
    PutAscii(packet, 97, "Bidder1");
    PutU8(packet, 107, 5U); // note length
    PutAscii(packet, 108, "Hello");
    PutU8(packet, 188, 15U); // item data length
    PutU8(packet, 189, 0xCD); // first item data byte

    const auto detail = AuctionDetailResponse::Parse(packet);

    REQUIRE(detail.has_value());
    CHECK(detail->RequestId == 99U);
    CHECK(detail->ListingId == 555ULL);
    CHECK(detail->Version == 4U);
    CHECK(detail->Category == AuctionCategory::Armor);
    CHECK(detail->CurrentPrice.Scalar() == 2000U);
    CHECK(detail->BuyoutPrice.Scalar() == 9000U);
    CHECK(detail->BidCount == 4U);
    CHECK(detail->OriginalEndsAt == 1700000000U);
    CHECK(detail->EndsAt == 1700003600U);
    CHECK(detail->SellerName == L"Seller1");
    CHECK(detail->CurrentBidderName == L"Bidder1");
    CHECK(detail->NoteLength == 5U);
    CHECK(detail->Note == L"Hello");
    CHECK(detail->ItemDataLength == 15U);
    CHECK(detail->ItemData[0] == 0xCD);
}

TEST_CASE("a detail response clamps an out-of-range note length to the field width [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionDetailResponse::FullWireLength, 0);
    PutU8(packet, 107, 255U); // claims far more than the 80-byte note field actually holds
    PutAscii(packet, 108, "Short");

    const auto detail = AuctionDetailResponse::Parse(packet);

    REQUIRE(detail.has_value());
    CHECK(detail->NoteLength == 255U);
    // Reading clamps to 80 bytes; the note field is zero-filled past "Short" in this packet, so the
    // trailing zero byte stops the read right after it rather than reading garbage or going out of bounds.
    CHECK(detail->Note == L"Short");
}

TEST_CASE("a truncated detail response fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionDetailResponse::FullWireLength - 1, 0);

    CHECK_FALSE(AuctionDetailResponse::Parse(packet).has_value());
}

namespace
{
    void PutMailboxEntry(std::vector<uint8_t>& buffer, size_t offset, uint64_t collectionId)
    {
        PutU64(buffer, offset + 0, collectionId);
        PutU8(buffer, offset + 8, static_cast<uint8_t>(AuctionCollectionKind::SaleProceeds));
        PutU8(buffer, offset + 9, static_cast<uint8_t>(AuctionCollectionStatus::Pending));
        PutU8(buffer, offset + 10, static_cast<uint8_t>(AuctionCurrencyMode::Zen));
        PutU32(buffer, offset + 11, 4000U); // original scalar
        PutU32(buffer, offset + 35, 4000U); // remaining scalar
        PutU64(buffer, offset + 59, 321ULL); // source listing id
        PutU8(buffer, offset + 67, 0U); // has item: false
        PutU8(buffer, offset + 68, 0U); // item data length
        PutU32(buffer, offset + 84, 2U); // version
    }
}

TEST_CASE("a mailbox entry reads its identifiers, amounts and source listing [ui][auction_wire]")
{
    std::vector<uint8_t> entry(AuctionMailboxEntry::WireLength, 0);
    PutMailboxEntry(entry, 0, 909ULL);

    const auto parsed = AuctionMailboxEntry::Parse(entry);

    REQUIRE(parsed.has_value());
    CHECK(parsed->CollectionId == 909ULL);
    CHECK(parsed->CollectionKind == AuctionCollectionKind::SaleProceeds);
    CHECK(parsed->CollectionStatus == AuctionCollectionStatus::Pending);
    CHECK_FALSE(parsed->Original.IsFruitBasket());
    CHECK(parsed->Original.Scalar() == 4000U);
    CHECK(parsed->Remaining.Scalar() == 4000U);
    CHECK(parsed->SourceListingId == 321ULL);
    CHECK_FALSE(parsed->HasItem);
    CHECK(parsed->Version == 2U);
}

TEST_CASE("a truncated mailbox entry fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> entry(AuctionMailboxEntry::WireLength - 1, 0);

    CHECK_FALSE(AuctionMailboxEntry::Parse(entry).has_value());
}

TEST_CASE("a mailbox response reads its header and every entry [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionMailboxResponse::FixedWireLength + 2 * AuctionMailboxEntry::WireLength, 0);
    PutU32(packet, 5, 11U); // request id
    PutU8(packet, 9, static_cast<uint8_t>(AuctionResult::Success));
    PutU16(packet, 10, 1U); // page
    PutU16(packet, 12, 1U); // total pages
    PutU32(packet, 14, 2U); // total count
    PutU32(packet, 18, 1700000900U); // server time
    PutU8(packet, 22, 2U); // entry count
    PutMailboxEntry(packet, 23, 1ULL);
    PutMailboxEntry(packet, 23 + AuctionMailboxEntry::WireLength, 2ULL);

    const auto response = AuctionMailboxResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->RequestId == 11U);
    CHECK(response->TotalCount == 2U);
    REQUIRE(response->Entries.size() == 2U);
    CHECK(response->Entries[0].CollectionId == 1ULL);
    CHECK(response->Entries[1].CollectionId == 2ULL);
}

TEST_CASE("a mailbox response truncated mid-entry fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionMailboxResponse::FixedWireLength + AuctionMailboxEntry::WireLength, 0);
    PutU8(packet, 22, 2U);
    PutMailboxEntry(packet, 23, 1ULL);

    CHECK_FALSE(AuctionMailboxResponse::Parse(packet).has_value());
}

TEST_CASE("a mailbox response shorter than its own fixed header fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionMailboxResponse::FixedWireLength - 1, 0);

    CHECK_FALSE(AuctionMailboxResponse::Parse(packet).has_value());
}

namespace
{
    void PutCurrencyDescriptor(std::vector<uint8_t>& buffer, size_t offset, AuctionCurrencyMode mode, bool enabled)
    {
        PutU8(buffer, offset + 0, static_cast<uint8_t>(mode));
        PutU8(buffer, offset + 1, enabled ? 1U : 0U);
        PutU32(buffer, offset + 2, 1000U); // minimum starting price
        PutU32(buffer, offset + 6, 2000000000U); // maximum starting price
        PutU32(buffer, offset + 10, 1000U); // minimum increment
        PutU8(buffer, offset + 14, 5U); // increment percent
        PutU32(buffer, offset + 15, 50000U); // spendable scalar
        PutU16(buffer, offset + 39, 10U); // small packed-jewel denomination
        PutU16(buffer, offset + 41, 20U); // medium packed-jewel denomination
        PutU16(buffer, offset + 43, 30U); // large packed-jewel denomination
    }
}

TEST_CASE("a currency descriptor reads its rules and spendable units [ui][auction_wire]")
{
    std::vector<uint8_t> entry(AuctionCurrencyDescriptor::WireLength, 0);
    PutCurrencyDescriptor(entry, 0, AuctionCurrencyMode::Zen, true);

    const auto descriptor = AuctionCurrencyDescriptor::Parse(entry);

    REQUIRE(descriptor.has_value());
    CHECK(descriptor->CurrencyMode == AuctionCurrencyMode::Zen);
    CHECK(descriptor->Enabled);
    CHECK(descriptor->MinimumStartingPrice == 1000U);
    CHECK(descriptor->MaximumStartingPrice == 2000000000U);
    CHECK(descriptor->MinimumIncrement == 1000U);
    CHECK(descriptor->IncrementPercent == 5U);
    CHECK_FALSE(descriptor->Spendable.IsFruitBasket());
    CHECK(descriptor->Spendable.Scalar() == 50000U);
    CHECK(descriptor->PackUnits == std::array<uint16_t, 3>{10U, 20U, 30U});
}

TEST_CASE("a truncated currency descriptor fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> entry(AuctionCurrencyDescriptor::LegacyWireLength - 1, 0);

    CHECK_FALSE(AuctionCurrencyDescriptor::Parse(entry).has_value());
}

TEST_CASE("an open response reads its fees, duration rules and every currency descriptor [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionOpenResponse::FixedWireLength + 2 * AuctionCurrencyDescriptor::WireLength, 0);
    PutU32(packet, 5, 7U); // request id
    PutU8(packet, 9, static_cast<uint8_t>(AuctionResult::Success));
    PutU32(packet, 10, 1700001000U); // server time
    PutU32(packet, 14, 2U); // configuration version with packed-jewel metadata
    PutU16(packet, 18, 100U); // listing fee basis points
    PutU16(packet, 20, 500U); // success fee basis points
    PutU8(packet, 22, 0x0F); // duration mask
    PutU8(packet, 23, 24U); // default duration hours
    PutU8(packet, 24, 8U); // page size
    PutU16(packet, 25, 2U); // pending mailbox count
    PutU8(packet, 27, 2U); // currency count
    PutCurrencyDescriptor(packet, 28, AuctionCurrencyMode::Zen, true);
    PutCurrencyDescriptor(packet, 28 + AuctionCurrencyDescriptor::WireLength, AuctionCurrencyMode::Chaos, false);

    const auto response = AuctionOpenResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->RequestId == 7U);
    CHECK(response->ServerTime == 1700001000U);
    CHECK(response->ConfigurationVersion == 2U);
    CHECK(response->ListingFeeBasisPoints == 100U);
    CHECK(response->SuccessFeeBasisPoints == 500U);
    CHECK(response->DurationMask == 0x0F);
    CHECK(response->DefaultDurationHours == 24U);
    CHECK(response->PageSize == 8U);
    CHECK(response->PendingMailboxCount == 2U);
    REQUIRE(response->Currencies.size() == 2U);
    CHECK(response->Currencies[0].CurrencyMode == AuctionCurrencyMode::Zen);
    CHECK(response->Currencies[0].Enabled);
    CHECK(response->Currencies[1].CurrencyMode == AuctionCurrencyMode::Chaos);
    CHECK_FALSE(response->Currencies[1].Enabled);
}

TEST_CASE("an open response truncated mid-descriptor fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionOpenResponse::FixedWireLength + AuctionCurrencyDescriptor::WireLength, 0);
    PutU32(packet, 14, 2U);
    PutU8(packet, 27, 2U);
    PutCurrencyDescriptor(packet, 28, AuctionCurrencyMode::Zen, true);

    CHECK_FALSE(AuctionOpenResponse::Parse(packet).has_value());
}

TEST_CASE("an open response accepts legacy currency descriptors without packed-jewel metadata [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionOpenResponse::FixedWireLength + AuctionCurrencyDescriptor::LegacyWireLength, 0);
    PutU8(packet, 27, 1U);
    PutU8(packet, 28, static_cast<uint8_t>(AuctionCurrencyMode::Chaos));
    PutU8(packet, 29, 1U);
    PutU32(packet, 30, 1U);

    const auto response = AuctionOpenResponse::Parse(packet);

    REQUIRE(response.has_value());
    REQUIRE(response->Currencies.size() == 1U);
    CHECK(response->Currencies[0].CurrencyMode == AuctionCurrencyMode::Chaos);
    CHECK(response->Currencies[0].PackUnits == std::array<uint16_t, 3>{0U, 0U, 0U});
}

TEST_CASE("an open response shorter than its own fixed header fails to parse [ui][auction_wire]")
{
    std::vector<uint8_t> packet(AuctionOpenResponse::FixedWireLength - 1, 0);

    CHECK_FALSE(AuctionOpenResponse::Parse(packet).has_value());
}
