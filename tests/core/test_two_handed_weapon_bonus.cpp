#include "stdafx.h"
#include "doctest.h"
#include "GameLogic/Items/TwoHandedWeaponBonus.h"

using GameLogic::Items::HasTwoHandedWeaponBonus;

TEST_CASE("Two-handed bonus icon accepts melee weapons and staffs")
{
    CHECK(HasTwoHandedWeaponBonus(ITEM_SWORD, true, -1));
    CHECK(HasTwoHandedWeaponBonus(ITEM_AXE, true, -1));
    CHECK(HasTwoHandedWeaponBonus(ITEM_MACE, true, -1));
    CHECK(HasTwoHandedWeaponBonus(ITEM_SPEAR, true, -1));
    CHECK(HasTwoHandedWeaponBonus(ITEM_STAFF, true, -1));
}

TEST_CASE("Two-handed bonus icon excludes dual wield and occupied offhand")
{
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_SWORD, false, ITEM_SWORD));
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_SWORD, true, ITEM_SWORD));
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_STAFF, true, ITEM_SHIELD));
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_SWORD, false, -1));
}

TEST_CASE("Two-handed bonus icon excludes bows and invalid item categories")
{
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_BOW, true, -1));
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_SHIELD, true, -1));
    CHECK_FALSE(HasTwoHandedWeaponBonus(ITEM_HELM, true, -1));
    CHECK_FALSE(HasTwoHandedWeaponBonus(-1, true, -1));
}
