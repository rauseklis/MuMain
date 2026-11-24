#include "stdafx.h"
#include "doctest.h"
#include "GameLogic/Items/PlusThirteenBonus.h"

using GameLogic::Items::EquippedPiece;
using GameLogic::Items::HasPlusThirteenSet;

namespace
{
constexpr int BoneSet = 4;

void FillSet(EquippedPiece (&pieces)[5], int setIndex, int level)
{
    pieces[0] = {ITEM_HELM + setIndex, level, 10};
    pieces[1] = {ITEM_ARMOR + setIndex, level, 10};
    pieces[2] = {ITEM_PANTS + setIndex, level, 10};
    pieces[3] = {ITEM_GLOVES + setIndex, level, 10};
    pieces[4] = {ITEM_BOOTS + setIndex, level, 10};
}
} // namespace

TEST_CASE("A full +13 set of the same kind shows the bonus")
{
    EquippedPiece pieces[5];
    FillSet(pieces, BoneSet, 13);
    CHECK(HasPlusThirteenSet(pieces));

    pieces[1].level = 15;
    CHECK(HasPlusThirteenSet(pieces));
}

TEST_CASE("One piece below +13 hides the bonus")
{
    EquippedPiece pieces[5];
    FillSet(pieces, BoneSet, 13);
    pieces[2].level = 12;
    CHECK_FALSE(HasPlusThirteenSet(pieces));
}

TEST_CASE("A missing or broken piece hides the bonus")
{
    EquippedPiece pieces[5];
    FillSet(pieces, BoneSet, 13);
    pieces[3] = {-1, 0, 0};
    CHECK_FALSE(HasPlusThirteenSet(pieces));

    FillSet(pieces, BoneSet, 13);
    pieces[4].durability = 0;
    CHECK_FALSE(HasPlusThirteenSet(pieces));
}

TEST_CASE("Pieces of different sets hide the bonus")
{
    EquippedPiece pieces[5];
    FillSet(pieces, BoneSet, 13);
    pieces[0].type = ITEM_HELM + BoneSet + 1;
    CHECK_FALSE(HasPlusThirteenSet(pieces));
}

TEST_CASE("Items in the wrong slot do not count")
{
    EquippedPiece pieces[5];
    FillSet(pieces, BoneSet, 13);
    pieces[1].type = ITEM_PANTS + BoneSet;
    CHECK_FALSE(HasPlusThirteenSet(pieces));
}

TEST_CASE("Only a weapon of at least +13 gets the weapon bonus")
{
    using GameLogic::Items::IsPlusThirteenWeapon;
    CHECK(IsPlusThirteenWeapon(ITEM_SWORD + 3, 13));
    CHECK(IsPlusThirteenWeapon(ITEM_STAFF + 1, 15));
    CHECK_FALSE(IsPlusThirteenWeapon(ITEM_SWORD + 3, 12));
    CHECK_FALSE(IsPlusThirteenWeapon(ITEM_SHIELD + 1, 13));
    CHECK_FALSE(IsPlusThirteenWeapon(ITEM_HELM + BoneSet, 13));
    CHECK_FALSE(IsPlusThirteenWeapon(-1, 13));
}
