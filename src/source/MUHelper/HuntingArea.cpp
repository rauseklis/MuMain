#include "HuntingArea.h"

#include <cmath>

namespace MUHelper::HuntingArea
{

int Distance(Tile from, Tile to)
{
    const int dx = from.x - to.x;
    const int dy = from.y - to.y;
    return static_cast<int>(std::ceil(std::sqrt(static_cast<double>(dx * dx + dy * dy))));
}

bool Contains(Tile start, int huntingDistance, Tile monster)
{
    return Distance(start, monster) <= huntingDistance;
}

bool ShouldDefendAgainst(bool selfDefense, bool isAttacking, bool insideArea, bool longRangeCounterAttack)
{
    if (!selfDefense || !isAttacking)
    {
        return false;
    }

    return insideArea || longRangeCounterAttack;
}

} // namespace MUHelper::HuntingArea
