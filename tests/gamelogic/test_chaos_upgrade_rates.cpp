#include "doctest.h"
#include "GameLogic/Items/ChaosUpgradeRates.h"

namespace Upgrade = GameLogic::Items::ChaosUpgrade;

TEST_CASE("Chaos upgrade preview matches all cumulative server modifiers")
{
    const int recipes[] = {3, 4, 22, 23, 49, 50};
    const int bases[] = {60, 60, 55, 55, 20, 20};
    for (int i = 0; i < 6; ++i)
    {
        for (int mask = 0; mask < 32; ++mask)
        {
            const bool luck = (mask & 1) != 0;
            const bool excellent = (mask & 2) != 0;
            const bool ancient = (mask & 4) != 0;
            const bool guardian = (mask & 8) != 0;
            const bool socket = (mask & 16) != 0;
            const int expected =
                std::max(0, bases[i] + (luck ? 20 : 0) - 10 * (excellent + ancient + guardian) - (socket ? 20 : 0));
            CHECK(Upgrade::Rate(recipes[i], luck, excellent, ancient, guardian, socket) == expected);
        }
    }
    CHECK(Upgrade::Rate(1, true, false, false, false, false) == Upgrade::kUnsupportedRecipe);
}

TEST_CASE("Guardian category follows server definitions rather than the added option")
{
    CHECK(Upgrade::IsGuardianType(0, 22));
    CHECK(Upgrade::IsGuardianType(5, 19));
    CHECK(Upgrade::IsGuardianType(7, 29));
    CHECK(Upgrade::IsGuardianType(11, 43));
    CHECK(Upgrade::IsGuardianType(8, 73));
    CHECK_FALSE(Upgrade::IsGuardianType(10, 73));
    CHECK_FALSE(Upgrade::IsGuardianType(0, 21));
    CHECK_FALSE(Upgrade::IsGuardianType(12, 29));
    CHECK_FALSE(Upgrade::IsGuardianType(8, 44));
}
