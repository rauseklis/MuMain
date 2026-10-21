#include <doctest.h>

#include <array>
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

TEST_CASE("the kinds are the server's forty-five codes in order")
{
    for (int i = 0; i < KIND_COUNT; ++i)
    {
        CHECK(KindAt(i).Code == i + 1);
        REQUIRE(FindKind(static_cast<uint8_t>(i + 1)) != nullptr);
        CHECK(FindKind(static_cast<uint8_t>(i + 1)) == &KindAt(i));
    }

    CHECK(FindKind(0) == nullptr);
    CHECK(FindKind(46) == nullptr);
    CHECK(std::wstring(KindAt(1).Name) == L"Jewel of Bless");
    CHECK(std::wstring(KindAt(13).Name) == L"Fruit of Strength");
    CHECK(std::wstring(KindAt(15).Name) == L"Chaos Bundle x10");
    CHECK(std::wstring(KindAt(25).Name) == L"Chaos Bundle x20");
    CHECK(std::wstring(KindAt(44).Name) == L"Higher Refining Bundle x30");
}

TEST_CASE("every page has fifteen rows and together the pages show every kind once")
{
    std::array<int, KIND_COUNT> seen{};
    for (int page = 0; page < PAGE_COUNT; ++page)
    {
        for (int row = 0; row < ROW_COUNT; ++row)
        {
            const int kind = KindOnPage(page, row);
            REQUIRE(kind >= 0);
            REQUIRE(kind < KIND_COUNT);
            ++seen[kind];
            CHECK(PageOf(kind) == page);
            CHECK(RowOnPage(kind) == row);
        }
    }
    for (const int count : seen)
    {
        CHECK(count == 1);
    }
}

TEST_CASE("a jewel's three bundles sit together on one page")
{
    CHECK(std::wstring(KindAt(KindOnPage(1, 0)).Name) == L"Chaos Bundle x10");
    CHECK(std::wstring(KindAt(KindOnPage(1, 1)).Name) == L"Chaos Bundle x20");
    CHECK(std::wstring(KindAt(KindOnPage(1, 2)).Name) == L"Chaos Bundle x30");
    CHECK(std::wstring(KindAt(KindOnPage(1, 14)).Name) == L"Creation Bundle x30");
    CHECK(std::wstring(KindAt(KindOnPage(2, 0)).Name) == L"Guardian Bundle x10");
    CHECK(std::wstring(KindAt(KindOnPage(2, 14)).Name) == L"Higher Refining Bundle x30");
    CHECK(std::wstring(PageName(0)) == L"Jewels & fruits");
    CHECK(std::wstring(PageName(2)) == L"Bundles");
}

TEST_CASE("request amounts are written as little-endian 32-bit values per kind")
{
    Amounts amounts{};
    amounts[0] = 0x01020304u;
    amounts[44] = 7;
    std::vector<uint8_t> bytes(KIND_COUNT * 4, 0xFF);

    AppendRequestAmounts(amounts, bytes.data());

    CHECK(bytes[0] == 0x04);
    CHECK(bytes[3] == 0x01);
    CHECK(bytes[4] == 0);
    CHECK(bytes[44 * 4] == 7);
    CHECK(bytes[44 * 4 + 3] == 0);
}

TEST_CASE("a state response is read at the documented offsets")
{
    std::vector<uint8_t> packet(StateResponse::FullWireLength, 0);
    packet[5] = static_cast<uint8_t>(Result::Success);
    packet[6] = 1;
    PutU32(packet, 7, 1234567);
    PutU32(packet, 7 + 4 * 44, 42);
    PutU32(packet, 187, 7);
    PutU32(packet, 187 + 4 * 44, 99);

    const auto response = StateResponse::Parse(packet);

    REQUIRE(response.has_value());
    CHECK(response->Result == Result::Success);
    CHECK(response->AutoDeposit);
    CHECK(response->Counts[0] == 1234567);
    CHECK(response->Counts[1] == 0);
    CHECK(response->Counts[44] == 42);
    CHECK(response->Inventory[0] == 7);
    CHECK(response->Inventory[1] == 0);
    CHECK(response->Inventory[44] == 99);
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
    unknown[5] = 200;
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

TEST_CASE("a pickup notification is read at the documented offsets")
{
    std::vector<uint8_t> packet(PickupNotification::FullWireLength, 0);
    packet[4] = 2;
    PutU32(packet, 5, 1);
    PutU64(packet, 9, 1204);

    const auto notification = PickupNotification::Parse(packet);

    REQUIRE(notification.has_value());
    CHECK(notification->KindCode == 2);
    CHECK(notification->Units == 1);
    CHECK(notification->NewCount == 1204);
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

TEST_CASE("an amount is limited to what is available and to the per-request maximum")
{
    CHECK(ClampAmount(5u, 100) == 5u);
    CHECK(ClampAmount(500u, 100) == 100u);
    CHECK(ClampAmount(5u, 0) == 0u);
    CHECK(ClampAmount(std::nullopt, 100) == 0u);
    CHECK(ClampAmount(4000000000u, 5000000000ULL) == MAX_WITHDRAW_UNITS);
}

TEST_CASE("ticking a row selects everything available, un-ticking clears it")
{
    CHECK(ToggleAmount(0, 12) == 12u);
    CHECK(ToggleAmount(5, 12) == 0u);
    CHECK(ToggleAmount(0, 0) == 0u);
    CHECK(ToggleAmount(0, 5000000000ULL) == MAX_WITHDRAW_UNITS);
}

TEST_CASE("select all takes every kind which has something available")
{
    Available available{};
    available[0] = 3;
    available[4] = 250000;
    available[14] = 1;

    const Amounts amounts = SelectAll(available);

    CHECK(amounts[0] == 3u);
    CHECK(amounts[1] == 0u);
    CHECK(amounts[4] == MAX_WITHDRAW_UNITS);
    CHECK(amounts[14] == 1u);
    CHECK(SelectedKinds(amounts) == 3);
    CHECK(TotalAmount(amounts) == 3u + MAX_WITHDRAW_UNITS + 1u);
}

TEST_CASE("amounts shrink when the available counts shrink, and empty rows stay empty")
{
    Available available{};
    available[0] = 10;
    available[1] = 2;
    Amounts amounts{};
    amounts[0] = 8;
    amounts[1] = 5;
    amounts[2] = 0;

    LimitToAvailable(amounts, available);

    CHECK(amounts[0] == 8u);
    CHECK(amounts[1] == 2u);
    CHECK(amounts[2] == 0u);

    available[0] = 3;
    LimitToAvailable(amounts, available);
    CHECK(amounts[0] == 3u);
}

TEST_CASE("every result has a message and Success is the only cheerful one")
{
    for (uint8_t code = 0; code <= static_cast<uint8_t>(Result::ServiceUnavailable); ++code)
    {
        CHECK(std::wstring(ResultText(static_cast<Result>(code))).size() > 0);
    }
    CHECK(std::wstring(ResultText(Result::Success)) == L"Done.");
}
