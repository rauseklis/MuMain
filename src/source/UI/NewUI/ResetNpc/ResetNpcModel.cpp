#include "UI/NewUI/ResetNpc/ResetNpcModel.h"

namespace ResetNpc
{
    namespace
    {
        constexpr size_t OutcomeOffset = 4;
        constexpr size_t FlagsOffset = 5;
        constexpr size_t NextResetOffset = 6;
        constexpr size_t MaximumResetsOffset = 8;
        constexpr size_t RequiredLevelOffset = 10;
        constexpr size_t CurrentLevelOffset = 12;
        constexpr size_t RequiredZenOffset = 14;
        constexpr size_t CurrentZenOffset = 18;
        constexpr size_t RequiredFruitsOffset = 22;
        constexpr size_t BankFruitsOffset = 26;
        constexpr size_t WornItemsOffset = 30;
        constexpr int DigitsPerGroup = 3;
        constexpr uint8_t HighestOutcome = static_cast<uint8_t>(Outcome::Refused);

        uint16_t ReadU16(std::span<const uint8_t> packet, size_t offset)
        {
            return static_cast<uint16_t>(packet[offset] | (packet[offset + 1] << 8));
        }

        uint32_t ReadU32(std::span<const uint8_t> packet, size_t offset)
        {
            return static_cast<uint32_t>(packet[offset]) | (static_cast<uint32_t>(packet[offset + 1]) << 8)
                | (static_cast<uint32_t>(packet[offset + 2]) << 16) | (static_cast<uint32_t>(packet[offset + 3]) << 24);
        }
    }

    std::optional<State> State::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        State state;
        state.Result = packet[OutcomeOffset] <= HighestOutcome ? static_cast<Outcome>(packet[OutcomeOffset]) : Outcome::Refused;
        state.Flags = packet[FlagsOffset];
        state.NextResetNumber = ReadU16(packet, NextResetOffset);
        state.MaximumResets = ReadU16(packet, MaximumResetsOffset);
        state.RequiredLevel = ReadU16(packet, RequiredLevelOffset);
        state.CurrentLevel = ReadU16(packet, CurrentLevelOffset);
        state.RequiredZen = ReadU32(packet, RequiredZenOffset);
        state.CurrentZen = ReadU32(packet, CurrentZenOffset);
        state.RequiredFruits = ReadU32(packet, RequiredFruitsOffset);
        state.BankFruits = ReadU32(packet, BankFruitsOffset);
        state.WornItems = packet[WornItemsOffset];
        return state;
    }

    bool State::CanReset() const
    {
        return LevelEnough() && ZenEnough() && FruitsEnough() && GearOk() && !LimitReached();
    }

    std::wstring FormatNumber(uint64_t value)
    {
        const std::wstring digits = std::to_wstring(value);
        std::wstring result;
        const size_t length = digits.size();
        for (size_t i = 0; i < length; ++i)
        {
            if (i > 0 && (length - i) % DigitsPerGroup == 0)
            {
                result += L',';
            }
            result += digits[i];
        }
        return result;
    }

    std::wstring HeadingText(const State& state)
    {
        if (state.LimitReached())
        {
            return L"Maximum resets reached";
        }

        std::wstring text = L"Reset " + std::to_wstring(state.NextResetNumber);
        if (state.MaximumResets > 0)
        {
            text += L" of " + std::to_wstring(state.MaximumResets);
        }
        return text;
    }

    const wchar_t* OutcomeText(Outcome outcome)
    {
        switch (outcome)
        {
        case Outcome::Done:
            return L"Your character has been reset.";
        case Outcome::RequirementsNotMet:
            return L"You do not meet the requirements of this reset yet.";
        case Outcome::LimitReached:
            return L"You have reached the maximum number of resets.";
        case Outcome::Refused:
            return L"The reset is not available right now. Please try again.";
        case Outcome::Info:
        default:
            return L"";
        }
    }
}
