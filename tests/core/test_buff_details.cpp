#include "doctest.h"
#include "GameLogic/Buffs/BuffDetails.h"
#include "UI/NewUI/HUD/BuffDuration.h"

namespace
{
    std::vector<std::uint8_t> Packet(int effect, int action, std::uint32_t remaining, const std::string& text = {})
    {
        const auto length = 13 + (text.empty() ? 0 : text.size() + 2);
        std::vector<std::uint8_t> bytes(length);
        bytes[0] = 0xC2;
        bytes[1] = static_cast<std::uint8_t>(length >> 8);
        bytes[2] = static_cast<std::uint8_t>(length);
        bytes[3] = GameLogic::Buffs::BuffDetails::PacketCode;
        bytes[4] = GameLogic::Buffs::BuffDetails::ProtocolVersion;
        bytes[5] = static_cast<std::uint8_t>(action);
        bytes[6] = static_cast<std::uint8_t>(effect);
        bytes[7] = static_cast<std::uint8_t>(effect >> 8);
        for (int byte = 0; byte < sizeof(remaining); ++byte)
        {
            bytes[8 + byte] = static_cast<std::uint8_t>(remaining >> (byte * 8));
        }
        bytes[12] = text.empty() ? 0 : 1;
        if (!text.empty())
        {
            bytes[13] = 1;
            bytes[14] = static_cast<std::uint8_t>(text.size());
            std::copy(text.begin(), text.end(), bytes.begin() + 15);
        }
        return bytes;
    }
}

TEST_CASE("Buff detail snapshots refresh countdown and replace cast values")
{
    GameLogic::Buffs::BuffDetails details;
    REQUIRE(details.Apply(Packet(81, 1, 30, "Maximum HP: -10%"), 1000));
    REQUIRE(details.Find(81));
    CHECK(details.Find(81)->lines[0].text == "Maximum HP: -10%");
    CHECK(details.Find(81)->lines[0].penalty);
    CHECK_FALSE(details.IsExpired(81, 30999));
    CHECK(details.IsExpired(81, 31000));
    REQUIRE(details.Apply(Packet(81, 1, 60, "Maximum HP: -20%"), 20000));
    CHECK(details.Find(81)->expiresAt == 80000);
    CHECK(details.Find(81)->lines[0].text == "Maximum HP: -20%");
}

TEST_CASE("Buff detail removal and map reset discard stale owner values")
{
    GameLogic::Buffs::BuffDetails details;
    REQUIRE(details.Apply(Packet(4, 1, 1000), 0));
    REQUIRE(details.Apply(Packet(4, 0, 0), 0));
    CHECK(details.Find(4) == nullptr);
    REQUIRE(details.Apply(Packet(130, 1, 1000), 0));
    REQUIRE(details.Apply(Packet(0, 2, 0), 0));
    CHECK(details.Find(130) == nullptr);
}

TEST_CASE("Malformed buff detail packets never replace a valid snapshot")
{
    GameLogic::Buffs::BuffDetails details;
    const auto valid = Packet(1, 1, 1, "Attack damage: +125");
    REQUIRE(details.Apply(valid, 0));
    for (std::size_t length = 0; length < valid.size(); ++length)
    {
        CHECK_FALSE(details.Apply(std::span(valid).first(length), 500));
    }
    auto invalid = valid;
    invalid[4] = 3;
    CHECK_FALSE(details.Apply(invalid, 0));
    invalid = valid;
    invalid[6] = 200;
    CHECK_FALSE(details.Apply(invalid, 0));
    invalid = valid;
    invalid[14] = 97;
    CHECK_FALSE(details.Apply(invalid, 0));
    CHECK(details.Find(1)->expiresAt == 1000);
    CHECK(details.Find(1)->lines[0].text == "Attack damage: +125");
}

TEST_CASE("Shop buff countdowns support long rentals and legacy millisecond packets")
{
    GameLogic::Buffs::BuffDetails details;
    constexpr std::uint32_t NinetyDaysSeconds = 90 * 86400;
    REQUIRE(details.Apply(Packet(40, 1, NinetyDaysSeconds, "Normal EXP: +50%"), 1000));
    CHECK(details.Find(40)->expiresAt == 1000ULL + NinetyDaysSeconds * 1000ULL);
    CHECK(UI::Buffs::FormatDuration(details.Find(40)->expiresAt, 1000) == L"Remaining: 90d 00:00:00");
    CHECK(UI::Buffs::FormatDuration(1, 0) == L"Remaining: 00:00:01");
    CHECK(UI::Buffs::FormatDuration(1, 2) == L"Remaining: 00:00:00");
    auto legacy = Packet(40, 1, 30000);
    legacy[4] = 1;
    REQUIRE(details.Apply(legacy, 1000));
    CHECK(details.Find(40)->expiresAt == 31000);
}
