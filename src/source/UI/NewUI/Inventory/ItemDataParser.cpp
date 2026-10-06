#include "UI/NewUI/Inventory/ItemDataParser.h"

ItemCreationParams ParseItemData(std::span<const BYTE> itemData)
{
    ItemCreationParams params = {};

    if (itemData.size() < 5)
        return params;

    params.Group = (itemData[0] >> 4) & 0xF;
    params.Number = ((itemData[0] & 0xF) << 8) + itemData[1];
    params.Level = itemData[2];
    params.Durability = itemData[3];
    auto flags = static_cast<ItemOptionFlags>(itemData[4]);
    params.WithLuck = flags & ItemOptionFlags::HasLuck;
    params.WithSkill = flags & ItemOptionFlags::HasSkill;
    params.HasGuardianOption = flags & ItemOptionFlags::HasGuardian;

    // Every optional read is bounds-checked; truncated or malformed data leaves IsValid false.
    size_t offset = 5;
    if (flags & ItemOptionFlags::HasOption)
    {
        if (itemData.size() < offset + 1)
            return params;

        params.OptionLevel = itemData[offset] & 0xF;
        params.OptionType = (itemData[offset] >> 4) & 0xF;
        offset++;
    }

    if (flags & ItemOptionFlags::HasExcellent)
    {
        if (itemData.size() < offset + 1)
            return params;

        params.ExcellentFlags = itemData[offset];
        offset++;
    }

    if (flags & ItemOptionFlags::HasAncient)
    {
        if (itemData.size() < offset + 1)
            return params;

        params.AncientDiscriminator = itemData[offset] & 0xF;
        params.AncientBonusOption = (itemData[offset] >> 4) & 0xF;
        offset++;
    }

    if (flags & ItemOptionFlags::HasHarmony)
    {
        if (itemData.size() < offset + 1)
            return params;

        params.HasHarmonyOption = true;
        params.HarmonyOptionLevel = itemData[offset] & 0xF;
        params.HarmonyOptionType = (itemData[offset] >> 4) & 0xF;
        offset++;
    }

    if (flags & ItemOptionFlags::HasSockets)
    {
        if (itemData.size() < offset + 1)
            return params;

        params.SocketBonusOption = (itemData[offset] >> 4) & 0xF;
        params.SocketCount = itemData[offset] & 0xF;
        if (params.SocketCount > MAX_SOCKETS)
            return params;

        if (itemData.size() < offset + 1 + params.SocketCount)
            return params;

        for (int i = 0; i < params.SocketCount; ++i)
        {
            params.SocketOptions[i] = itemData[offset + 1 + i];
        }
    }

    params.IsValid = true;
    return params;
}
