#pragma once

#include <algorithm>

namespace GameLogic::Items::ChaosUpgrade
{
constexpr int kUnsupportedRecipe = -1;
constexpr int kLuckBonus = 20;
constexpr int kCategoryPenalty = 10;
constexpr int kSocketPenalty = 20;

constexpr int BaseRate(int recipeId)
{
    switch (recipeId)
    {
    case 3:
    case 4:
        return 60; // +10, +11
    case 22:
    case 23:
        return 55; // +12, +13
    case 49:
    case 50:
        return 20; // +14, +15
    default:
        return kUnsupportedRecipe;
    }
}

// S6 item definitions which support Guardian options, even before adding one.
constexpr bool IsGuardianType(int group, int number)
{
    constexpr int kSwordGroup = 0;
    constexpr int kMaceGroup = 2;
    constexpr int kBowGroup = 4;
    constexpr int kStaffGroup = 5;
    constexpr int kHelmGroup = 7;
    constexpr int kBootsGroup = 11;
    constexpr int kGlovesGroup = 10;
    constexpr int kFirstGuardianSet = 29;
    constexpr int kLastGuardianSet = 33;
    constexpr int kAuraSet = 43;
    constexpr int kPhoenixSoulSet = 73;
    constexpr int kBoneBlade = 22;
    constexpr int kExplosionBlade = 23;
    constexpr int kPhoenixSoulStar = 35;
    constexpr int kSoleilScepter = 14;
    constexpr int kSylphWindBow = 21;
    constexpr int kViperStaff = 12;
    constexpr int kStormBlitzStick = 19;
    switch (group)
    {
    case kSwordGroup:
        return number == kBoneBlade || number == kExplosionBlade || number == kPhoenixSoulStar;
    case kMaceGroup:
        return number == kSoleilScepter;
    case kBowGroup:
        return number == kSylphWindBow;
    case kStaffGroup:
        return number == kViperStaff || number == kStormBlitzStick;
    default:
        return group >= kHelmGroup && group <= kBootsGroup &&
               ((number >= kFirstGuardianSet && number <= kLastGuardianSet) || number == kAuraSet ||
                (number == kPhoenixSoulSet && group != kGlovesGroup));
    }
}

constexpr int Rate(int recipeId, bool luck, bool excellent, bool ancient, bool guardian, bool socket)
{
    const int base = BaseRate(recipeId);
    if (base == kUnsupportedRecipe)
        return kUnsupportedRecipe;
    const int penalties = kCategoryPenalty * (excellent + ancient + guardian) + (socket ? kSocketPenalty : 0);
    return std::clamp(base + (luck ? kLuckBonus : 0) - penalties, 0, 100);
}
} // namespace GameLogic::Items::ChaosUpgrade
