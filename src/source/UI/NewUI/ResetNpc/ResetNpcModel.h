#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

// The pure side of the window of the reset NPC (Raymond the Reset Master): the server's answer (0xD7 sub 0x80) with its parser,
// the requirement flags and the texts. No window state and no drawing, so it is unit tested without the game.
namespace ResetNpc
{
    // What the server's answer tells (ResetNpcOutcome on the server).
    enum class Outcome : uint8_t
    {
        Info = 0,
        Done = 1,
        RequirementsNotMet = 2,
        LimitReached = 3,
        Refused = 4,
    };

    // The bits of the flags byte: which requirements are met.
    constexpr uint8_t FLAG_LEVEL = 1;
    constexpr uint8_t FLAG_ZEN = 2;
    constexpr uint8_t FLAG_FRUITS = 4;
    constexpr uint8_t FLAG_LIMIT_REACHED = 8;
    // Nothing is worn (or the rule does not apply), and whether the reset has the rule that nothing may be worn.
    constexpr uint8_t FLAG_GEAR_OK = 16;
    constexpr uint8_t FLAG_GEAR_RULE = 32;

    // The server's answer, a C1 packet: Outcome@4, Flags@5, NextResetNumber@6, MaximumResets@8, RequiredLevel@10, CurrentLevel@12
    // (all u16), RequiredZen@14, CurrentZen@18, RequiredFruits@22, BankFruits@26 (all u32), WornItems@30 (u8).
    struct State
    {
        static constexpr size_t FullWireLength = 31;

        Outcome Result = Outcome::Refused;
        uint8_t Flags = 0;
        uint16_t NextResetNumber = 0;
        uint16_t MaximumResets = 0;
        uint16_t RequiredLevel = 0;
        uint16_t CurrentLevel = 0;
        uint32_t RequiredZen = 0;
        uint32_t CurrentZen = 0;
        uint32_t RequiredFruits = 0;
        uint32_t BankFruits = 0;
        uint8_t WornItems = 0;

        [[nodiscard]] static std::optional<State> Parse(std::span<const uint8_t> packet);

        [[nodiscard]] bool LevelEnough() const { return (Flags & FLAG_LEVEL) != 0; }
        [[nodiscard]] bool ZenEnough() const { return (Flags & FLAG_ZEN) != 0; }
        [[nodiscard]] bool FruitsEnough() const { return (Flags & FLAG_FRUITS) != 0; }
        [[nodiscard]] bool LimitReached() const { return (Flags & FLAG_LIMIT_REACHED) != 0; }
        [[nodiscard]] bool GearOk() const { return (Flags & FLAG_GEAR_OK) != 0; }
        // The reset has the rule that nothing may be worn.
        [[nodiscard]] bool GearRuleActive() const { return (Flags & FLAG_GEAR_RULE) != 0; }
        // Every requirement is met and the limit is not reached: the Reset button may be pressed.
        [[nodiscard]] bool CanReset() const;
    };

    // The number with a comma between the thousands: 25,000,000.
    [[nodiscard]] std::wstring FormatNumber(uint64_t value);

    // "Reset 7 of 50", or "Reset 7" when there is no limit, or the sentence for the reached limit.
    [[nodiscard]] std::wstring HeadingText(const State& state);

    // A sentence about what the answer meant, for the line under the requirements; empty for plain information.
    [[nodiscard]] const wchar_t* OutcomeText(Outcome outcome);
}
