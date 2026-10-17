#include <doctest.h>
#include "Render/Items/RoyalMaceFlameProfile.h"

TEST_CASE("Royal flames are cosmetic, bounded and increase with upgrade level")
{
    using Render::Items::RoyalFlames::ForItem;
    constexpr int ItemsPerGroup = 512;
    constexpr int Royal = 2 * ItemsPerGroup + 23;
    CHECK(ForItem(Royal - 1, ItemsPerGroup, 13).emitters == 0);
    CHECK(ForItem(0, ItemsPerGroup, 13).emitters == 0);
    CHECK(ForItem(Royal, 0, 13).emitters == 0);
    CHECK(ForItem(Royal, ItemsPerGroup, -1).emitters == 0);
    CHECK(ForItem(Royal, ItemsPerGroup, 0).emitters == 0);
    CHECK(ForItem(Royal, ItemsPerGroup, 6).emitters == 0);
    CHECK(ForItem(Royal, ItemsPerGroup, 7).emitters == 1);
    CHECK(ForItem(Royal, ItemsPerGroup, 9).emitters == 2);
    CHECK(ForItem(Royal, ItemsPerGroup, 11).emitters == 2);
    CHECK(ForItem(Royal, ItemsPerGroup, 13).emitters == 3);
    for (int level = 8; level <= 15; ++level)
        CHECK(ForItem(Royal, ItemsPerGroup, level).scale > ForItem(Royal, ItemsPerGroup, level - 1).scale);
    CHECK(ForItem(Royal, ItemsPerGroup, 100).scale == ForItem(Royal, ItemsPerGroup, 15).scale);
    CHECK(ForItem(Royal, ItemsPerGroup, 100).emitters == 3);
}
