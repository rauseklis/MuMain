#pragma once

namespace GameLogic::Items::QuickDrop
{
// Why a shift+right-click on an inventory item is (not) turned into an immediate drop.
enum class Verdict
{
    Allowed,
    HeroDead,
    HighValue,
    Reinforced,
    DropBanned,
    CursedTempleLostMap,
};

// The facts about the clicked item and the player the verdict depends on, so the rules
// can be tested without an ITEM or the game state.
struct Candidate
{
    bool heroDead = false;
    bool highValue = false;
    bool reinforced = false;
    bool dropBanned = false;
    bool cursedTempleLostMap = false;
};

// The same rules the drag-and-drop to the ground applies, in the order it reports them.
Verdict Evaluate(const Candidate& candidate);

// Only the bag slots can be quick-dropped, never the equipped gear slots.
bool IsBagSlot(int slotIndex, int firstBagSlot, int endBagSlot);
} // namespace GameLogic::Items::QuickDrop
