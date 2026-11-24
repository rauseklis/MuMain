#pragma once

namespace GameLogic::Items
{
// The level from which an armor piece counts for the +13 set bonus.
constexpr int PlusThirteenLevel = 13;

// The Vitality the full set adds; the server grants the same amount, so the character window shows the real total.
constexpr int PlusThirteenSetVitality = 100;

struct EquippedPiece
{
    int type;
    int level;
    int durability;
};

// True when all five armor pieces (helm, armor, pants, gloves, boots, in this order) belong to the same set and
// are at least +13. The server grants the bonus (+100 Vitality, 5% ignore defence); this only decides what to show.
bool HasPlusThirteenSet(const EquippedPiece (&pieces)[5]);

// True for a weapon (swords to staffs, not shields) of at least +13 in a hand. The server rolls the red strike for it.
bool IsPlusThirteenWeapon(int type, int level);
} // namespace GameLogic::Items
