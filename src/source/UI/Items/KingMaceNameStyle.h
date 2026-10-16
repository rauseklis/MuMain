#pragma once

#include "GameLogic/Items/KingMaceRules.h"

namespace UI::Items
{
    inline constexpr unsigned char KingMaceNameRed = 230;
    inline constexpr unsigned char KingMaceNameGreen = 107;
    inline constexpr unsigned char KingMaceNameBlue = 10;

    constexpr bool HasKingMaceNameStyle(int itemType, int itemsPerGroup)
    {
        return GameLogic::Items::IsKingMace(itemType, itemsPerGroup);
    }
}
