#include "CherryBlossomBranches.h"

namespace GameLogic::Items::CherryBlossom
{
namespace
{
constexpr int ItemGroupOffset = 14 * 512;
constexpr int WhiteBranch = ItemGroupOffset + 88;
constexpr int RedBranch = ItemGroupOffset + 89;
constexpr int GoldenBranch = ItemGroupOffset + 90;
constexpr int WhiteCapacity = 10;
constexpr int RedCapacity = 30;
constexpr int GoldenCapacity = 255;
} // namespace

bool CanStack(int sourceType, int targetType, int sourceCount, int targetCount)
{
    if (sourceType != targetType || sourceCount <= 0 || targetCount <= 0)
    {
        return false;
    }

    int capacity = 0;
    switch (sourceType)
    {
    case WhiteBranch:
        capacity = WhiteCapacity;
        break;
    case RedBranch:
        capacity = RedCapacity;
        break;
    case GoldenBranch:
        capacity = GoldenCapacity;
        break;
    default:
        return false;
    }
    return sourceCount < capacity && targetCount < capacity;
}
} // namespace GameLogic::Items::CherryBlossom
