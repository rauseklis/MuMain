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
