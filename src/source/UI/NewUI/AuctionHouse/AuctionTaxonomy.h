// <copyright file="AuctionTaxonomy.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>

namespace AuctionHouse
{
    // The wire value which means every category (and, in the sub-category byte, every sub-category).
    inline constexpr uint8_t WireAll = 0xFF;

    struct SubCategoryInfo
    {
        uint8_t wire;
        const wchar_t* label;
    };

    // A browse category in rail order. The ids mirror the server's AuctionTaxonomy; the client never classifies. A
    // category's icon is the atlas cell with its rail position (rail index - 1).
    struct CategoryInfo
    {
        uint8_t wire;
        std::span<const SubCategoryInfo> subs;
    };

    // The categories below "All Categories", in rail order (Miscellaneous last).
    [[nodiscard]] std::span<const CategoryInfo> RailCategories() noexcept;

    // Rail index 0 is "All Categories"; 1 to RailCategories().size() are the categories.
    [[nodiscard]] const CategoryInfo* CategoryForRailIndex(int railIndex) noexcept;
    [[nodiscard]] uint8_t WireCategoryForRailIndex(int railIndex) noexcept;
    [[nodiscard]] const SubCategoryInfo* FindSubCategory(const CategoryInfo& category, uint8_t wire) noexcept;

    // The text of a rail row: the picked leaf's name when one of this category is picked, else the category's own label.
    [[nodiscard]] std::wstring RailRowText(const wchar_t* categoryLabel, const CategoryInfo* category, uint8_t selectedSub);

    struct Rect
    {
        int x;
        int y;
        int width;
        int height;

        [[nodiscard]] bool Contains(int px, int py) const noexcept
        {
            return px >= x && px < x + width && py >= y && py < y + height;
        }
    };

    struct FlyoutLayout
    {
        Rect panel;
        int firstRowY;
        int rowHeight;
        int rowCount;
        int padding;
    };

    // Places the flyout: its left edge at panelX, level with anchorTop, with one row for "All" plus one per leaf. It is
    // moved up when it would pass bottomLimit and down when it would pass topLimit.
    [[nodiscard]] FlyoutLayout LayoutFlyout(int panelX, int anchorTop, int leafCount, int rowHeight, int width, int padding,
        int topLimit, int bottomLimit) noexcept;

    // The row (0 is "All") under the point, or -1 when it is over the padding or outside the panel.
    [[nodiscard]] int FlyoutRowAt(const FlyoutLayout& layout, int x, int y) noexcept;

    using MeasureText = std::function<int(const std::wstring&)>;

    // Cuts the text and appends "..." until it fits maxWidth. Never returns an empty string.
    [[nodiscard]] std::wstring EllipsizeToWidth(const std::wstring& text, int maxWidth, const MeasureText& measure);
}
