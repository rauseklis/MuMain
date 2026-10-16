#include "stdafx.h"
#include "GameLogic/Items/TwoHandedWeaponBonus.h"

namespace GameLogic::Items
{
    bool HasTwoHandedWeaponBonus(int rightWeaponType, bool twoHanded, int leftWeaponType)
    {
        if (!twoHanded || leftWeaponType != -1)
        {
            return false;
        }

        const bool meleeWeapon = rightWeaponType >= ITEM_SWORD && rightWeaponType < ITEM_BOW;
        const bool staff = rightWeaponType >= ITEM_STAFF && rightWeaponType < ITEM_SHIELD;
        return meleeWeapon || staff;
    }
}
