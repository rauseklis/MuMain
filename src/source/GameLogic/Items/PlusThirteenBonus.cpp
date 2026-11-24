#include "stdafx.h"
#include "GameLogic/Items/PlusThirteenBonus.h"

namespace GameLogic::Items
{
bool HasPlusThirteenSet(const EquippedPiece (&pieces)[5])
{
    const int firstGroup = ITEM_GROUP_HELM;
    const int setIndex = pieces[0].type % MAX_ITEM_INDEX;

    for (int slot = 0; slot < 5; ++slot)
    {
        const EquippedPiece& piece = pieces[slot];
        if (piece.type < 0 || piece.type / MAX_ITEM_INDEX != firstGroup + slot)
        {
            return false;
        }

        if (piece.level < PlusThirteenLevel || piece.durability <= 0)
        {
            return false;
        }

        if (piece.type % MAX_ITEM_INDEX != setIndex)
        {
            return false;
        }
    }

    return true;
}
} // namespace GameLogic::Items
