#include "stdafx.h"
#include "GameLogic/Items/PotionStacking.h"

namespace GameLogic::Items
{
    namespace
    {
        constexpr int RecoveryStackLimit = 50;
        constexpr int ShieldStackLimit = 3;
    }

    int RecoveryPotionStackLimit(int itemType)
    {
        if ((itemType >= ITEM_APPLE && itemType <= ITEM_LARGE_MANA_POTION) || itemType == ITEM_ANTIDOTE)
        {
            return RecoveryStackLimit;
        }

        if (itemType >= ITEM_SMALL_SHIELD_POTION && itemType <= ITEM_LARGE_COMPLEX_POTION)
        {
            return ShieldStackLimit;
        }

        return 0;
    }

    bool CanStackRecoveryPotions(const ITEM& source, const ITEM& target)
    {
        if (source.Type != target.Type || source.Level != target.Level)
        {
            return false;
        }

        const int limit = RecoveryPotionStackLimit(source.Type);
        // A full source can still partially merge into a non-full target.
        return limit > 0 && source.Durability > 0 && source.Durability <= limit &&
               target.Durability > 0 && target.Durability < limit;
    }
}
