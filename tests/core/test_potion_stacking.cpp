#include "stdafx.h"
#include "doctest.h"
#include "GameLogic/Items/PotionStacking.h"

namespace
{
    ITEM Potion(int type, int count, int level = 0)
    {
        ITEM item{};
        item.Type = static_cast<short>(type);
        item.Durability = static_cast<BYTE>(count);
        item.Level = level;
        return item;
    }
}

TEST_CASE("HP, mana, apples and antidotes stack to fifty")
{
    using namespace GameLogic::Items;
    for (int type = ITEM_APPLE; type <= ITEM_ANTIDOTE; ++type)
    {
        if (type == ITEM_SIEGE_POTION)
            continue;

        CHECK(RecoveryPotionStackLimit(type) == 50);
        CHECK(CanStackRecoveryPotions(Potion(type, 3), Potion(type, 3)));
        CHECK(CanStackRecoveryPotions(Potion(type, 1), Potion(type, 49)));
        CHECK(CanStackRecoveryPotions(Potion(type, 50), Potion(type, 1)));
        CHECK_FALSE(CanStackRecoveryPotions(Potion(type, 1), Potion(type, 50)));
    }
}

TEST_CASE("SD and complex HP plus SD potions remain limited to three")
{
    using namespace GameLogic::Items;
    for (int type = ITEM_SMALL_SHIELD_POTION; type <= ITEM_LARGE_COMPLEX_POTION; ++type)
    {
        CHECK(RecoveryPotionStackLimit(type) == 3);
        CHECK(CanStackRecoveryPotions(Potion(type, 1), Potion(type, 2)));
        CHECK(CanStackRecoveryPotions(Potion(type, 3), Potion(type, 1)));
        CHECK_FALSE(CanStackRecoveryPotions(Potion(type, 1), Potion(type, 3)));
        CHECK_FALSE(CanStackRecoveryPotions(Potion(type, 4), Potion(type, 1)));
    }
}

TEST_CASE("Potion stacking rejects mismatched, empty and unsupported items")
{
    using namespace GameLogic::Items;
    CHECK_FALSE(CanStackRecoveryPotions(Potion(ITEM_APPLE, 1), Potion(ITEM_ANTIDOTE, 1)));
    CHECK_FALSE(CanStackRecoveryPotions(Potion(ITEM_APPLE, 1, 1), Potion(ITEM_APPLE, 1)));
    CHECK_FALSE(CanStackRecoveryPotions(Potion(ITEM_APPLE, 0), Potion(ITEM_APPLE, 1)));
    CHECK_FALSE(CanStackRecoveryPotions(Potion(ITEM_APPLE, 1), Potion(ITEM_APPLE, 0)));
    CHECK_FALSE(CanStackRecoveryPotions(Potion(ITEM_APPLE, 51), Potion(ITEM_APPLE, 1)));
    CHECK(RecoveryPotionStackLimit(ITEM_SIEGE_POTION) == 0);
    CHECK(RecoveryPotionStackLimit(ITEM_ARROWS) == 0);
    CHECK(RecoveryPotionStackLimit(ITEM_ELIXIR_OF_STRENGTH) == 0);
    CHECK_FALSE(CanStackRecoveryPotions(Potion(ITEM_SIEGE_POTION, 1), Potion(ITEM_SIEGE_POTION, 1)));
}
