#include "stdafx.h"
#include "GameLogic/Items/ShopRestrictions.h"

#include "Data/GameData/ItemData/ItemDatabase.h"
#include "GameLogic/Items/ItemCategories.h"
#include "GameLogic/Items/ItemLevelVariants.h"

// The rule flags come from the item data; the exceptions below depend on the
// item level.
namespace
{
using Data::Items::ItemAction;

bool IsAllowed(const ITEM* pItem, ItemAction action)
{
    return g_ItemDatabase.IsAllowed(pItem->Type, action);
}
} // namespace

namespace GameLogic::Items
{
    bool IsPersonalShopBan(const ITEM* pItem)
    {
        if (pItem == NULL)
        {
            return false;
        }

        if (pItem->bPeriodItem && !IsAllowed(pItem, ItemAction::SellInPersonalShopWhileRented))
        {
            return true;
        }
        if (IsSignOfLord(pItem))
        {
            return false;
        }
        if (IsHeartOfDarkLord(pItem) || IsWizardsRingVariant(pItem))
        {
            return true;
        }

        return !IsAllowed(pItem, ItemAction::SellInPersonalShop);
    }

    bool IsSellingBan(const ITEM* pItem)
    {
        const int Level = pItem->Level;

        if (pItem->bPeriodItem && pItem->bExpiredPeriod && IsAllowed(pItem, ItemAction::SellWhenRentalExpired))
        {
            return false;
        }

        // Every Wizard's Ring can be sold to an NPC: it can't be traded or stored, so selling is the
        // only way to free the inventory slot (a reset needs an empty inventory).
        if (pItem->Type == ITEM_WIZARDS_RING)
        {
            return false;
        }

        // Rena +1 (Stone) and Remedy of Love +1 to +5.
        if ((pItem->Type == ITEM_RENA && Level == 1) ||
            (pItem->Type == ITEM_REMEDY_OF_LOVE && Level >= 1 && Level <= 5))
        {
            return true;
        }

        return !IsAllowed(pItem, ItemAction::Sell);
    }

    bool IsRepairBan(const ITEM* pItem)
    {
        return !IsAllowed(pItem, ItemAction::Repair);
    }
}
