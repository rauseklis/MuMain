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
