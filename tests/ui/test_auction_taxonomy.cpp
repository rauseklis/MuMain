#include <doctest.h>

#include <set>
#include <string>

#include "UI/NewUI/AuctionHouse/AuctionTaxonomy.h"

using namespace AuctionHouse;

TEST_CASE("the rail lists eight categories with Miscellaneous last [ui][auction_taxonomy]")
{
    const auto categories = RailCategories();
    REQUIRE(categories.size() == 8);
    const uint8_t expectedWires[] = { 0, 1, 2, 3, 4, 5, 7, 6 };
    for (size_t i = 0; i < categories.size(); ++i)
    {
        CHECK(categories[i].wire == expectedWires[i]);
    }
    CHECK(categories.back().subs.empty());
}

TEST_CASE("rail index zero is every category and others map to wire values [ui][auction_taxonomy]")
{
    CHECK(WireCategoryForRailIndex(0) == WireAll);
    CHECK(WireCategoryForRailIndex(1) == 0);
    CHECK(WireCategoryForRailIndex(2) == 1);
    CHECK(WireCategoryForRailIndex(7) == 7);
    CHECK(WireCategoryForRailIndex(8) == 6);
    CHECK(WireCategoryForRailIndex(9) == WireAll);
    CHECK(WireCategoryForRailIndex(-1) == WireAll);
    CHECK(CategoryForRailIndex(0) == nullptr);
    CHECK(CategoryForRailIndex(9) == nullptr);
    CHECK(CategoryForRailIndex(2)->wire == 1);
}

TEST_CASE("every category's leaves have unique ids and labels [ui][auction_taxonomy]")
{
    const size_t expectedLeaves[] = { 6, 6, 2, 6, 5, 3, 2, 0 };
    const auto categories = RailCategories();
    for (size_t i = 0; i < categories.size(); ++i)
    {
        CHECK(categories[i].subs.size() == expectedLeaves[i]);
        std::set<uint8_t> ids;
        std::set<std::wstring> labels;
        for (const SubCategoryInfo& leaf : categories[i].subs)
        {
            CHECK(leaf.wire != WireAll);
            CHECK(std::wstring(leaf.label).size() > 0);
            ids.insert(leaf.wire);
            labels.insert(leaf.label);
        }
        CHECK(ids.size() == categories[i].subs.size());
        CHECK(labels.size() == categories[i].subs.size());
    }
}

TEST_CASE("leaf ids follow the server table [ui][auction_taxonomy]")
{
    const CategoryInfo& armor = *CategoryForRailIndex(2);
    CHECK(FindSubCategory(armor, 3) != nullptr);
    CHECK(std::wstring(FindSubCategory(armor, 3)->label) == L"Gloves");
    CHECK(std::wstring(FindSubCategory(armor, 5)->label) == L"Shields");
    CHECK(FindSubCategory(armor, 6) == nullptr);
    CHECK(FindSubCategory(armor, WireAll) == nullptr);
}

TEST_CASE("the rail row shows the picked leaf of its own category only [ui][auction_taxonomy]")
{
    const CategoryInfo& armor = *CategoryForRailIndex(2);
    CHECK(RailRowText(L"Armor", &armor, 3) == L"Gloves");
    CHECK(RailRowText(L"Armor", &armor, WireAll) == L"Armor");
    CHECK(RailRowText(L"Armor", &armor, 99) == L"Armor");
    CHECK(RailRowText(L"All Categories", nullptr, 3) == L"All Categories");
}

TEST_CASE("a flyout meets the rail and sits level with its row [ui][auction_taxonomy]")
{
    const FlyoutLayout layout = LayoutFlyout(128, 122, 6, 17, 132, 3, 105, 388);
    CHECK(layout.rowCount == 7);
    CHECK(layout.panel.x == 128);
    CHECK(layout.panel.y == 122);
    CHECK(layout.panel.width == 132);
    CHECK(layout.panel.height == 125);
    CHECK(layout.firstRowY == 125);
}

TEST_CASE("flyout stays inside the limits [ui][auction_taxonomy]")
{
    const FlyoutLayout low = LayoutFlyout(128, 360, 6, 17, 132, 3, 105, 388);
    CHECK(low.panel.y + low.panel.height == 388);

    const FlyoutLayout high = LayoutFlyout(128, 100, 1, 17, 132, 3, 105, 388);
    CHECK(high.panel.y == 105);
}

TEST_CASE("flyout rows are found by position [ui][auction_taxonomy]")
{
    const FlyoutLayout layout = LayoutFlyout(128, 122, 6, 17, 132, 3, 105, 388);
    CHECK(FlyoutRowAt(layout, 140, 125) == 0);
    CHECK(FlyoutRowAt(layout, 140, 142) == 1);
    CHECK(FlyoutRowAt(layout, 140, 243) == 6);
    CHECK(FlyoutRowAt(layout, 140, 244) == -1);
    CHECK(FlyoutRowAt(layout, 129, 130) == -1);
    CHECK(FlyoutRowAt(layout, 140, 124) == -1);
}

TEST_CASE("long labels are cut with dots to fit [ui][auction_taxonomy]")
{
    const auto measure = [](const std::wstring& text) { return static_cast<int>(text.size()) * 6; };
    CHECK(EllipsizeToWidth(L"Bows & Crossbows", 100, measure) == L"Bows & Crossbows");
    CHECK(EllipsizeToWidth(L"Bows & Crossbows", 60, measure) == L"Bows &...");
    CHECK(EllipsizeToWidth(L"Bows & Crossbows", 10, measure) == L"...");
}
