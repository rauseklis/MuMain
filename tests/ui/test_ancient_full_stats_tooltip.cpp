#include "App/stdafx.h"
#include "GameLogic/Items/CSItemOption.h"
#include "I18N/All.h"

#include <doctest.h>
#include <array>
#include <filesystem>
#include <string>

extern wchar_t TextList[50][100];
extern int TextListColor[50];

TEST_CASE("An unworn Tier 1 Ancient item displays four inactive full-set stat options")
{
    const auto original = std::filesystem::current_path();
    std::filesystem::current_path(MU_ANCIENT_TEST_ASSETS);
    g_strSelectedML = L"Eng";
    const bool loaded = g_csItemOption.OpenItemSetScript();
    std::filesystem::current_path(original);
    REQUIRE(loaded);
    REQUIRE_FALSE(g_csItemOption.IsAncientSetEquipped());

    struct Sample
    {
        int type;
        int variant;
        int bonus;
    };
    constexpr int itemGroupSize = 512;
    const std::array<Sample, 4> samples = {{{8 * itemGroupSize, 1, 100},
                                            {7 * itemGroupSize + 2, 2, 100},
                                            {9 * itemGroupSize + 11, 2, 100},
                                            {7 * itemGroupSize + 8, 1, 75}}};
    for (const auto sample : samples)
    {
        CAPTURE(sample.type);
        ITEM item{};
        item.Type = sample.type;
        item.AncientDiscriminator = sample.variant;
        const int lines = g_csItemOption.RenderSetOptionListInItem(&item, 0, false);
        bool fullHeading = false;
        for (int row = 0; row < lines; ++row)
            fullHeading |= std::wstring(TextList[row]) == I18N::Game::FullSetOptions;
        CHECK(fullHeading);
        for (const auto stat : {0, 1, 3, 2})
        {
            wchar_t expected[100]{};
            std::swprintf(expected, std::size(expected), I18N::Game::Lookup(950 + stat), sample.bonus);
            bool shownInactive = false;
            for (int row = 0; row < lines; ++row)
                shownInactive |= std::wstring(TextList[row]) == expected && TextListColor[row] == TEXT_COLOR_GRAY;
            CHECK(shownInactive);
        }
    }
    CHECK_FALSE(g_csItemOption.IsAncientSetEquipped());
}
