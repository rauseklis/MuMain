#include <doctest.h>
#include "UI/Items/KingMaceNameStyle.h"

TEST_CASE("Only the two king maces receive orange name styling [ui][items]")
{
    constexpr int itemsPerGroup = 512;
    for (int itemType = 0; itemType < 16 * itemsPerGroup; ++itemType)
    {
        const bool expected = itemType == 2 * itemsPerGroup + 22
            || itemType == 2 * itemsPerGroup + 23;
        CHECK(UI::Items::HasKingMaceNameStyle(itemType, itemsPerGroup) == expected);
    }
    CHECK_FALSE(UI::Items::HasKingMaceNameStyle(-1, itemsPerGroup));
    CHECK_FALSE(UI::Items::HasKingMaceNameStyle(0, 0));
    CHECK(UI::Items::KingMaceNameRed == 230);
    CHECK(UI::Items::KingMaceNameGreen == 107);
    CHECK(UI::Items::KingMaceNameBlue == 10);
}

TEST_CASE("King mace upgrade display matches the server at all levels [ui][items]")
{
    for (int level = 0; level <= 15; ++level)
    {
        CHECK(GameLogic::Items::KingMaceUpgradeDamage(level) == level * 4);
    }
    CHECK(GameLogic::Items::KingMaceUpgradeDamage(-1) == 0);
    CHECK(GameLogic::Items::KingMaceUpgradeDamage(16) == 60);
    for (int level = 0; level <= 15; ++level)
    {
        CHECK(GameLogic::Items::WeaponUpgradeDamage(2 * 512 + 22, 512, level) == level * 4);
        CHECK(GameLogic::Items::WeaponUpgradeDamage(2 * 512 + 23, 512, level) == level * 4);
        const int legacyBonus = (level < 9 ? level : 9) * 3 + (level > 9 ? level - 6 : 0);
        CHECK(GameLogic::Items::WeaponUpgradeDamage(16, 512, level) == legacyBonus);
    }
}
