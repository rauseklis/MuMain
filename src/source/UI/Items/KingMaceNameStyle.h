#pragma once

namespace UI::Items
{
    inline constexpr int KingMaceGroup = 2;
    inline constexpr int KingMaceNumber = 22;
    inline constexpr int RoyalKingMaceNumber = 23;
    inline constexpr unsigned char KingMaceNameRed = 230;
    inline constexpr unsigned char KingMaceNameGreen = 107;
    inline constexpr unsigned char KingMaceNameBlue = 10;

    constexpr bool HasKingMaceNameStyle(int itemType, int itemsPerGroup)
    {
        if (itemType < 0 || itemsPerGroup <= RoyalKingMaceNumber)
            return false;

        return itemType == KingMaceGroup * itemsPerGroup + KingMaceNumber
            || itemType == KingMaceGroup * itemsPerGroup + RoyalKingMaceNumber;
    }
}
