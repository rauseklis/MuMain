#include "GameLogic/Buffs/BuffDetails.h"

#include <chrono>

namespace GameLogic::Buffs
{
    bool BuffDetails::Apply(std::span<const std::uint8_t> packet, std::uint64_t now)
    {
        constexpr std::size_t HeaderLength = 13;
        if (packet.size() < HeaderLength || packet[0] != 0xC2 || packet[3] != PacketCode ||
            (packet[4] != 1 && packet[4] != ProtocolVersion) || ((packet[1] << 8) | packet[2]) != packet.size())
        {
            return false;
        }

        const int action = packet[5];
        const int effect = packet[6] | (packet[7] << 8);
        const int count = packet[12];
        if (action > 2 || count > MaximumLines || (action != 1 && (count != 0 || packet.size() != HeaderLength)))
        {
            return false;
        }
        if (action == 2)
        {
            if (effect != 0)
            {
                return false;
            }
            Clear();
            return true;
        }
        if (effect <= 0 || effect >= MaximumEffects)
        {
            return false;
        }

        std::uint32_t remaining = 0;
        for (int byte = 0; byte < sizeof(remaining); ++byte)
        {
            remaining |= static_cast<std::uint32_t>(packet[8 + byte]) << (byte * 8);
        }

        Effect update;
        update.known = action == 1;
        constexpr std::uint64_t MillisecondsPerSecond = 1000;
        update.expiresAt = now + static_cast<std::uint64_t>(remaining) * (packet[4] == 1 ? 1 : MillisecondsPerSecond);
        std::size_t offset = HeaderLength;
        for (int line = 0; line < count; ++line)
        {
            if (offset + 2 > packet.size())
            {
                return false;
            }
            const int color = packet[offset++];
            const std::size_t length = packet[offset++];
            if (color > 1 || length > MaximumTextBytes || offset + length > packet.size())
            {
                return false;
            }
            update.lines.push_back({std::string(reinterpret_cast<const char*>(packet.data() + offset), length), color == 1});
            offset += length;
        }
        if (offset != packet.size())
        {
            return false;
        }
        m_Effects[effect] = std::move(update);
        return true;
    }

    const BuffDetails::Effect* BuffDetails::Find(int effect) const
    {
        return effect > 0 && effect < MaximumEffects && m_Effects[effect].known ? &m_Effects[effect] : nullptr;
    }

    bool BuffDetails::IsExpired(int effect, std::uint64_t now) const
    {
        const auto* details = Find(effect);
        return details && now >= details->expiresAt;
    }

    void BuffDetails::Clear()
    {
        for (auto& effect : m_Effects)
        {
            effect = {};
        }
    }

    BuffDetails& ActiveBuffDetails()
    {
        static BuffDetails details;
        return details;
    }

    std::uint64_t BuffDetailsClock()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}
