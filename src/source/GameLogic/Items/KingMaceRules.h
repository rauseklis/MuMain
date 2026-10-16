#pragma once

namespace GameLogic::Items
{
    inline constexpr int KingMaceGroup = 2;
    inline constexpr int KingMaceNumber = 22;
    inline constexpr int RoyalKingMaceNumber = 23;
    inline constexpr int KingMaceDamagePerLevel = 4;

    constexpr bool IsKingMace(int itemType, int itemsPerGroup)
    {
        if (itemType < 0 || itemsPerGroup <= RoyalKingMaceNumber)
            return false;

        return itemType == KingMaceGroup * itemsPerGroup + KingMaceNumber
            || itemType == KingMaceGroup * itemsPerGroup + RoyalKingMaceNumber;
    }

    constexpr int KingMaceUpgradeDamage(int level)
    {
        return (level < 0 ? 0 : level > 15 ? 15 : level) * KingMaceDamagePerLevel;
    }

    constexpr int WeaponUpgradeDamage(int itemType, int itemsPerGroup, int level)
    {
        if (IsKingMace(itemType, itemsPerGroup))
            return KingMaceUpgradeDamage(level);

        constexpr int ordinaryFirstTierMaximum = 9;
        constexpr int ordinaryDamagePerLevel = 3;
        constexpr int ordinaryHighLevelOffset = 6;
        const int firstTierLevel = level < ordinaryFirstTierMaximum ? level : ordinaryFirstTierMaximum;
        return firstTierLevel * ordinaryDamagePerLevel
            + (level > ordinaryFirstTierMaximum ? level - ordinaryHighLevelOffset : 0);
    }
}
