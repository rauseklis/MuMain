#pragma once

namespace GameLogic::Items
{
// The level from which an armor piece counts for the +13 set bonus.
constexpr int PlusThirteenLevel = 13;

struct EquippedPiece
{
    int type;
    int level;
    int durability;
};

// True when all five armor pieces (helm, armor, pants, gloves, boots, in this order) belong to the same set and
// are at least +13. The server grants the bonus (+100 Vitality, 5% ignore defence); this only decides what to show.
bool HasPlusThirteenSet(const EquippedPiece (&pieces)[5]);
} // namespace GameLogic::Items
