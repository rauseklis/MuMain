#include "GameLogic/Items/QuickDrop.h"

namespace GameLogic::Items::QuickDrop
{
Verdict Evaluate(const Candidate& candidate)
{
    if (candidate.heroDead)
    {
        return Verdict::HeroDead;
    }
    if (candidate.reinforced)
    {
        return Verdict::Reinforced;
    }
    if (candidate.highValue)
    {
        return Verdict::HighValue;
    }
    if (candidate.dropBanned)
    {
        return Verdict::DropBanned;
    }
    if (candidate.cursedTempleLostMap)
    {
        return Verdict::CursedTempleLostMap;
    }

    return Verdict::Allowed;
}

bool IsBagSlot(int slotIndex, int firstBagSlot, int endBagSlot)
{
    return slotIndex >= firstBagSlot && slotIndex < endBagSlot;
}
} // namespace GameLogic::Items::QuickDrop
