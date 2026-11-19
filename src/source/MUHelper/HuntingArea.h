#pragma once

// The MU Helper's hunting area: the circle of the hunting range around the spot where the helper was started.
namespace MUHelper::HuntingArea
{

struct Tile
{
    int x;
    int y;
};

// Straight-line distance in tiles, rounded up.
[[nodiscard]] int Distance(Tile from, Tile to);

// True when the monster stands, or is walking to, a tile within huntingDistance of the start.
[[nodiscard]] bool Contains(Tile start, int huntingDistance, Tile monster, Tile monsterDestination);

// Self Defense switches to a monster only when it attacks the character, and only inside the hunting area unless
// Long-Range Counter Attack is on.
[[nodiscard]] bool ShouldDefendAgainst(bool selfDefense, bool isAttacking, bool insideArea,
                                       bool longRangeCounterAttack);

} // namespace MUHelper::HuntingArea
