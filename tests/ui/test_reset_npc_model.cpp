#include "doctest.h"

#include <array>

#include "UI/NewUI/ResetNpc/ResetNpcModel.h"

namespace
{
std::array<uint8_t, 30> MakePacket()
{
    std::array<uint8_t, 30> packet{};
    packet[0] = 0xC1;
    packet[1] = 30;
    packet[2] = 0xD7;
    packet[3] = 0x80;
    packet[4] = 0;                       // Info
    packet[5] = 0x03;                    // level and Zen are enough
    packet[6] = 7;                       // next reset number
    packet[8] = 50;                      // maximum resets
    packet[10] = 0x90; packet[11] = 0x01;  // required level 400
    packet[12] = 0x90; packet[13] = 0x01;  // current level 400
    packet[14] = 0x80; packet[15] = 0xF0; packet[16] = 0xFA; packet[17] = 0x02;  // 50,000,000
    packet[18] = 0x00; packet[19] = 0xE1; packet[20] = 0xF5; packet[21] = 0x05;  // 100,000,000
    packet[22] = 200;                    // required fruits
    packet[26] = 0xB8; packet[27] = 0x0B;  // 3000 fruits in the bank
    return packet;
}
}

TEST_CASE("the state is read from its fields")
{
    const auto packet = MakePacket();
    const auto state = ResetNpc::State::Parse(packet);
    REQUIRE(state.has_value());
    CHECK(state->Result == ResetNpc::Outcome::Info);
    CHECK(state->NextResetNumber == 7);
    CHECK(state->MaximumResets == 50);
    CHECK(state->RequiredLevel == 400);
    CHECK(state->CurrentLevel == 400);
    CHECK(state->RequiredZen == 50000000u);
    CHECK(state->CurrentZen == 100000000u);
    CHECK(state->RequiredFruits == 200u);
    CHECK(state->BankFruits == 3000u);
}

TEST_CASE("a short packet is not a state")
{
    const std::array<uint8_t, 12> shortPacket{};
    CHECK_FALSE(ResetNpc::State::Parse(shortPacket).has_value());
}

TEST_CASE("an unknown outcome is read as a refusal")
{
    auto packet = MakePacket();
    packet[4] = 99;
    const auto state = ResetNpc::State::Parse(packet);
    REQUIRE(state.has_value());
    CHECK(state->Result == ResetNpc::Outcome::Refused);
}

TEST_CASE("the reset button needs every requirement and no reached limit")
{
    auto packet = MakePacket();
    auto state = *ResetNpc::State::Parse(packet);
    CHECK_FALSE(state.CanReset());  // the fruits flag is not set

    packet[5] = ResetNpc::FLAG_LEVEL | ResetNpc::FLAG_ZEN | ResetNpc::FLAG_FRUITS;
    state = *ResetNpc::State::Parse(packet);
    CHECK(state.CanReset());

    packet[5] |= ResetNpc::FLAG_LIMIT_REACHED;
    state = *ResetNpc::State::Parse(packet);
    CHECK_FALSE(state.CanReset());
}

TEST_CASE("numbers get a comma between the thousands")
{
    CHECK(ResetNpc::FormatNumber(0) == L"0");
    CHECK(ResetNpc::FormatNumber(999) == L"999");
    CHECK(ResetNpc::FormatNumber(1000) == L"1,000");
    CHECK(ResetNpc::FormatNumber(25000000) == L"25,000,000");
    CHECK(ResetNpc::FormatNumber(125000000) == L"125,000,000");
}

TEST_CASE("the heading names the reset and the limit")
{
    auto packet = MakePacket();
    auto state = *ResetNpc::State::Parse(packet);
    CHECK(ResetNpc::HeadingText(state) == L"Reset 7 of 50");

    state.MaximumResets = 0;
    CHECK(ResetNpc::HeadingText(state) == L"Reset 7");

    state.Flags |= ResetNpc::FLAG_LIMIT_REACHED;
    CHECK(ResetNpc::HeadingText(state) == L"Maximum resets reached");
}
