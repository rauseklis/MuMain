// <copyright file="AuctionTaxonomy.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "UI/NewUI/AuctionHouse/AuctionTaxonomy.h"

namespace
{
    using AuctionHouse::SubCategoryInfo;

    constexpr SubCategoryInfo WeaponSubs[] = {
        { 0, L"Swords" }, { 1, L"Axes" }, { 2, L"Maces & Scepters" }, { 3, L"Spears" },
        { 4, L"Bows & Crossbows" }, { 5, L"Staves & Books" },
    };
    constexpr SubCategoryInfo ArmorSubs[] = {
        { 0, L"Helms" }, { 1, L"Armors" }, { 2, L"Pants" }, { 3, L"Gloves" }, { 4, L"Boots" }, { 5, L"Shields" },
    };
    constexpr SubCategoryInfo WingSubs[] = { { 0, L"Wings" }, { 1, L"Capes & Cloaks" } };
    constexpr SubCategoryInfo PetSubs[] = {
        { 0, L"Pets" }, { 1, L"Mounts" }, { 2, L"Rings" }, { 3, L"Pendants" }, { 4, L"Eggs" }, { 5, L"Seals & Tickets" },
    };
    constexpr SubCategoryInfo JewelSubs[] = {
        { 0, L"Jewels" }, { 1, L"Bundles" }, { 2, L"Refining stones" }, { 3, L"Seeds & Spheres" }, { 4, L"Fruits" },
    };
    constexpr SubCategoryInfo ConsumableSubs[] = {
        { 0, L"Potions" }, { 1, L"Elixirs & Buffs" }, { 2, L"Boxes & Misc" },
    };
    constexpr SubCategoryInfo ScrollSubs[] = { { 0, L"Orbs & Crystals" }, { 1, L"Scrolls & Parchments" } };

    // Rail order: Miscellaneous (wire 6) sits last, after Scrolls & Orbs (wire 7). Icons are items present in item.bmd:
    // Sword of Destruction, Brass Armor, Wings of Dragon, Dark Horse, Jewel of Bless, Large Healing Potion, Scroll of
    // Fire Ball and Box of Luck.
    const AuctionHouse::CategoryInfo Categories[] = {
        { 0, { 0, 16 }, WeaponSubs },
        { 1, { 8, 8 }, ArmorSubs },
        { 2, { 12, 5 }, WingSubs },
        { 3, { 13, 4 }, PetSubs },
        { 4, { 14, 13 }, JewelSubs },
        { 5, { 14, 3 }, ConsumableSubs },
        { 7, { 15, 3 }, ScrollSubs },
        { 6, { 14, 11 }, {} },
    };
}

namespace AuctionHouse
{
    std::span<const CategoryInfo> RailCategories() noexcept
    {
        return Categories;
    }

    const CategoryInfo* CategoryForRailIndex(int railIndex) noexcept
    {
        if (railIndex < 1 || railIndex > static_cast<int>(std::size(Categories)))
        {
            return nullptr;
        }
        return &Categories[railIndex - 1];
    }

    uint8_t WireCategoryForRailIndex(int railIndex) noexcept
    {
        const CategoryInfo* category = CategoryForRailIndex(railIndex);
        return category == nullptr ? WireAll : category->wire;
    }

    const SubCategoryInfo* FindSubCategory(const CategoryInfo& category, uint8_t wire) noexcept
    {
        for (const SubCategoryInfo& leaf : category.subs)
        {
            if (leaf.wire == wire)
            {
                return &leaf;
            }
        }
        return nullptr;
    }

    std::wstring RailRowText(const wchar_t* categoryLabel, const CategoryInfo* category, uint8_t selectedSub)
    {
        if (category != nullptr && selectedSub != WireAll)
        {
            if (const SubCategoryInfo* leaf = FindSubCategory(*category, selectedSub))
            {
                return leaf->label;
            }
        }
        return categoryLabel;
    }

    FlyoutLayout LayoutFlyout(int panelX, int anchorTop, int leafCount, int rowHeight, int width, int padding,
        int topLimit, int bottomLimit) noexcept
    {
        const int rows = leafCount + 1;
        const int height = rows * rowHeight + 2 * padding;
        int top = anchorTop;
        if (top + height > bottomLimit)
        {
            top = bottomLimit - height;
        }
        if (top < topLimit)
        {
            top = topLimit;
        }
        return FlyoutLayout{ Rect{ panelX, top, width, height }, top + padding, rowHeight, rows, padding };
    }

    int FlyoutRowAt(const FlyoutLayout& layout, int x, int y) noexcept
    {
        if (x < layout.panel.x + layout.padding || x >= layout.panel.x + layout.panel.width - layout.padding)
        {
            return -1;
        }
        const int offset = y - layout.firstRowY;
        if (offset < 0 || offset >= layout.rowCount * layout.rowHeight)
        {
            return -1;
        }
        return offset / layout.rowHeight;
    }

    std::wstring EllipsizeToWidth(const std::wstring& text, int maxWidth, const MeasureText& measure)
    {
        if (measure(text) <= maxWidth)
        {
            return text;
        }

        const std::wstring dots = L"...";
        for (size_t length = text.size(); length-- > 0;)
        {
            std::wstring head = text.substr(0, length);
            while (!head.empty() && head.back() == L' ')
            {
                head.pop_back();
            }
            const std::wstring candidate = head + dots;
            if (measure(candidate) <= maxWidth)
            {
                return candidate;
            }
        }
        return dots;
    }
}
