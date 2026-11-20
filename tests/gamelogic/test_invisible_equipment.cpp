#include "doctest.h"

#include "GameLogic/Items/InvisibleEquipment.h"

namespace Invisible = GameLogic::Items::InvisibleEquipment;
using Invisible::Look;

namespace
{
constexpr int ItemsPerGroup = 512;

constexpr int Item(int group, int number)
{
    return group * ItemsPerGroup + number;
}
} // namespace

TEST_CASE("the five pieces with number 74 in groups 7 to 11 are invisible")
{
    for (int group = 7; group <= 11; ++group)
    {
        CHECK(Invisible::IsInvisiblePiece(Item(group, 74), ItemsPerGroup));
        CHECK(Invisible::GetBodySlot(Item(group, 74), ItemsPerGroup) == group - 7);
    }
}

TEST_CASE("other items are not invisible")
{
    CHECK_FALSE(Invisible::IsInvisiblePiece(Item(6, 74), ItemsPerGroup));  // a shield number 74
    CHECK_FALSE(Invisible::IsInvisiblePiece(Item(12, 74), ItemsPerGroup)); // wings
    CHECK_FALSE(Invisible::IsInvisiblePiece(Item(9, 73), ItemsPerGroup));
    CHECK_FALSE(Invisible::IsInvisiblePiece(Item(9, 75), ItemsPerGroup));
    CHECK_FALSE(Invisible::IsInvisiblePiece(Item(8, 40), ItemsPerGroup)); // Red Wing Armor
    CHECK_FALSE(Invisible::IsInvisiblePiece(-1, ItemsPerGroup));          // empty slot
    CHECK(Invisible::GetLook(Item(8, 40), ItemsPerGroup, 9) == Look::NotInvisible);
}

TEST_CASE("+0 to +6 hide the body part")
{
    for (int level = 0; level <= 6; ++level)
    {
        CAPTURE(level);
        CHECK(Invisible::GetLook(Item(9, 74), ItemsPerGroup, level) == Look::Hidden);
    }
}

TEST_CASE("+7 to +15 show the golden body part")
{
    for (int level = 7; level <= 15; ++level)
    {
        CAPTURE(level);
        CHECK(Invisible::GetLook(Item(9, 74), ItemsPerGroup, level) == Look::GoldenBody);
    }
}

TEST_CASE("defense and stat requirements are fixed")
{
    CHECK(Invisible::FixedDefense == 200);
    CHECK(Invisible::FixedStatRequirement == 100);
}
