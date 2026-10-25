#include <utility>
#include <initializer_list>
#include <doctest.h>
#include "GameLogic/Items/CherryBlossomBranches.h"

TEST_CASE("Cherry Blossom stacks reach their exchange counts")
{
    using GameLogic::Items::CherryBlossom::CanStack;
    constexpr int offset = 14 * 512;
    for (const auto entry : {std::pair{88, 10}, std::pair{89, 30}, std::pair{90, 255}})
    {
        const int type = offset + entry.first;
        CHECK(CanStack(type, type, 1, entry.second - 1));
        CHECK_FALSE(CanStack(type, type, 1, entry.second));
        CHECK_FALSE(CanStack(type, type, entry.second, 1));
        CHECK_FALSE(CanStack(type, type, 0, 1));
    }
    CHECK_FALSE(CanStack(offset + 88, offset + 89, 1, 1));
    CHECK_FALSE(CanStack(offset + 85, offset + 85, 1, 1));
}
