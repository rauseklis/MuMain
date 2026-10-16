#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace GameLogic::Buffs
{
    class BuffDetails
    {
    public:
        static constexpr std::uint8_t PacketCode = 0xD7;
        static constexpr std::uint8_t ProtocolVersion = 2;
        static constexpr int MaximumEffects = 200;
        static constexpr int MaximumLines = 16;
        static constexpr int MaximumTextBytes = 96;

        struct Line
        {
            std::string text;
            bool penalty = false;
        };

        struct Effect
        {
            bool known = false;
            std::uint64_t expiresAt = 0;
            std::vector<Line> lines;
        };

        bool Apply(std::span<const std::uint8_t> packet, std::uint64_t now);
        const Effect* Find(int effect) const;
        bool IsExpired(int effect, std::uint64_t now) const;
        void Clear();

    private:
        std::array<Effect, MaximumEffects> m_Effects{};
    };

    BuffDetails& ActiveBuffDetails();
    std::uint64_t BuffDetailsClock();
}
