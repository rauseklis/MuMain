#pragma once

typedef struct tagITEM ITEM;

namespace GameLogic::Items
{
    // Zero means this item uses a different stacking rule.
    int RecoveryPotionStackLimit(int itemType);
    bool CanStackRecoveryPotions(const ITEM& source, const ITEM& target);
}
