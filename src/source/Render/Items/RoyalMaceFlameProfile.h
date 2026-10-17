#pragma once

#include "GameLogic/Items/KingMaceRules.h"

namespace Render::Items::RoyalFlames
{
struct Profile
{
    int emitters = 0;
    float scale = 0.f;
};

constexpr Profile ForItem(int itemType, int itemsPerGroup, int level)
{
    constexpr int FirstFlameLevel = 7;
    constexpr int MaximumLevel = 15;
    constexpr float StartingScale = 0.24f;
    constexpr float ScalePerLevel = 0.04f;
    if (itemsPerGroup <= GameLogic::Items::RoyalKingMaceNumber ||
        itemType != GameLogic::Items::KingMaceGroup * itemsPerGroup + GameLogic::Items::RoyalKingMaceNumber ||
        level < FirstFlameLevel)
        return {};
    const int boundedLevel = level > MaximumLevel ? MaximumLevel : level;
    return {1 + (boundedLevel >= 9 ? 1 : 0) + (boundedLevel >= 13 ? 1 : 0),
            StartingScale + (boundedLevel - FirstFlameLevel) * ScalePerLevel};
}
}
