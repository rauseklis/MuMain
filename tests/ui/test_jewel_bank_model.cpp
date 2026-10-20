#include <doctest.h>

#include <cstring>
#include <vector>

#include "UI/NewUI/JewelBank/JewelBankModel.h"

using namespace JewelBank;

namespace
{
    void PutU32(std::vector<uint8_t>& buffer, size_t offset, uint32_t value)
    {
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
    }

    void PutU64(std::vector<uint8_t>& buffer, size_t offset, uint64_t value)
    {
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
    }
} // namespace

TEST_CASE("the kinds are the server's fifteen codes in order")
{
    for (int i = 0; i < KIND_COUNT; ++i)
    {
        CHECK(KindAt(i).Code == i + 1);
        REQUIRE(FindKind(static_cast<uint8_t>(i + 1)) != nullptr);
        CHECK(FindKind(static_cast<uint8_t>(i + 1)) == &KindAt(i));
    }

    CHECK(FindKind(0) == nullptr);
    CHECK(FindKind(16) == nullptr);
    CHECK(std::wstring(KindAt(1).Name) == L"Jewel of Bless");
    CHECK(std::wstring(KindAt(13).Name) == L"Fruit of Strength");
}

TEST_CASE("a state response is read at the documented offsets")
{
    std::vector<uint8_t> packet(StateResponse::FullWireLength, 0);
    packet[4] = static_cast<uint8_t>(Result::Success);
    packet[5] = 1;
    PutU64(packet, 6, 1234567);
    PutU64(packet, 6 + 8 * 14, 42);

    const auto response = StateResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->Result == Result::Success);
    CHECK(response->AutoDeposit);
    CHECK(response->Counts[0] == 1234567);
    CHECK(response->Counts[1] == 0);
    CHECK(response->Counts[14] == 42);
}

TEST_CASE("a short or unknown packet is not a response")
{
    std::vector<uint8_t> shortState(StateResponse::FullWireLength - 1, 0);
    CHECK_FALSE(StateResponse::Parse(shortState).has_value());
    std::vector<uint8_t> shortOperation(OperationResponse::FullWireLength - 1, 0);
    CHECK_FALSE(OperationResponse::Parse(shortOperation).has_value());
    std::vector<uint8_t> shortPickup(PickupNotification::FullWireLength - 1, 0);
    CHECK_FALSE(PickupNotification::Parse(shortPickup).has_value());

    std::vector<uint8_t> unknown(StateResponse::FullWireLength, 0);
    unknown[4] = 200;
    const auto response = StateResponse::Parse(unknown);
    REQUIRE(response.has_value());
    CHECK(response->Result == Result::InvalidRequest);
}

TEST_CASE("an operation response carries its identifier, result and units")
{
    std::vector<uint8_t> packet(OperationResponse::FullWireLength, 0);
    for (size_t i = 0; i < 16; ++i)
    {
        packet[4 + i] = static_cast<uint8_t>(0xA0 + i);
    }
    packet[20] = static_cast<uint8_t>(Operation::Withdraw);
    packet[21] = static_cast<uint8_t>(Result::InventoryFull);
    packet[22] = 3;
    PutU32(packet, 23, 65);

    const auto response = OperationResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->OperationId[0] == 0xA0);
    CHECK(response->OperationId[15] == 0xAF);
    CHECK(response->Operation == Operation::Withdraw);
    CHECK(response->Result == Result::InventoryFull);
    CHECK(response->KindCode == 3);
    CHECK(response->Units == 65);
}

TEST_CASE("a pickup notification becomes a chat line")
{
    std::vector<uint8_t> packet(PickupNotification::FullWireLength, 0);
    packet[4] = 2;
    PutU32(packet, 5, 1);
    PutU64(packet, 9, 1204);

    const auto notification = PickupNotification::Parse(packet);

    REQUIRE(notification.has_value());
    CHECK(PickupMessage(*notification) == L"+1 Jewel of Bless -> Jewel Bank (1,204)");

    PickupNotification unknown;
    unknown.KindCode = 99;
    CHECK(PickupMessage(unknown).empty());
}

TEST_CASE("counts are written with thousands separators")
{
    CHECK(FormatCount(0) == L"0");
    CHECK(FormatCount(999) == L"999");
    CHECK(FormatCount(1000) == L"1,000");
    CHECK(FormatCount(1234567) == L"1,234,567");
    CHECK(FormatCount(1000000000000ULL) == L"1,000,000,000,000");
}

TEST_CASE("only a positive whole number is an amount")
{
    CHECK(ParseAmount(L"1") == 1u);
    CHECK(ParseAmount(L"250") == 250u);
    CHECK(ParseAmount(L"0007") == 7u);
    CHECK_FALSE(ParseAmount(L"").has_value());
    CHECK_FALSE(ParseAmount(L"0").has_value());
    CHECK_FALSE(ParseAmount(L"-5").has_value());
    CHECK_FALSE(ParseAmount(L"12a").has_value());
    CHECK_FALSE(ParseAmount(L"1 2").has_value());
    CHECK_FALSE(ParseAmount(L"4294967296").has_value());
    CHECK_FALSE(ParseAmount(L"99999999999").has_value());
}

TEST_CASE("a withdrawal is limited to the stored count and the per-request maximum")
{
    CHECK(WithdrawAmount(5u, 100) == 5u);
    CHECK(WithdrawAmount(500u, 100) == 100u);
    CHECK(WithdrawAmount(5u, 0) == 0u);
    CHECK(WithdrawAmount(std::nullopt, 100) == 0u);
    CHECK(WithdrawAmount(4000000000u, 5000000000ULL) == MAX_WITHDRAW_UNITS);
}

TEST_CASE("every result has a message and Success is the only cheerful one")
{
    for (uint8_t code = 0; code <= static_cast<uint8_t>(Result::ServiceUnavailable); ++code)
    {
        CHECK(std::wstring(ResultText(static_cast<Result>(code))).size() > 0);
    }
    CHECK(std::wstring(ResultText(Result::Success)) == L"Done.");
}
