#include "App/stdafx.h"
#include "Engine/Object/ZzzCharacter.h"
#include "GameLogic/Items/CSItemOption.h"
#include "I18N/All.h"

#include <doctest.h>
#include <array>
#include <filesystem>
#include <memory>
#include <string>

extern wchar_t TextList[50][100];
extern int TextListColor[50];

namespace
{
void CheckAncientSetNames()
{
    wchar_t name[64]{};
    REQUIRE(g_csItemOption.GetSetItemName(name, 8 * 512 + 40, 2));
    CHECK(std::wstring(name) == L"Semeden's ");
    REQUIRE(g_csItemOption.GetSetItemName(name, 7 * 512 + 40, 1));
    CHECK(std::wstring(name) == L"Chrono's ");
    REQUIRE(g_csItemOption.GetSetItemName(name, 8 * 512, 1));
    CHECK(std::wstring(name) == L"Hyperion's ");
}

void CheckEquippedFaseStats()
{
    constexpr int itemGroupSize = 512;
    // Exercise the same equipment recalculation that feeds the character window.
    auto machine = std::make_unique<CHARACTER_MACHINE>();
    auto hero = std::make_unique<CHARACTER>();
    auto itemAttributes = std::make_unique<ITEM_ATTRIBUTE[]>(MAX_ITEM);
    auto* previousMachine = CharacterMachine;
    auto* previousAttribute = CharacterAttribute;
    auto* previousHero = Hero;
    auto* previousItems = ItemAttribute;
    CharacterMachine = machine.get();
    CharacterAttribute = &machine->Character;
    Hero = hero.get();
    ItemAttribute = itemAttributes.get();
    Hero->Class = CLASS_ELF;
    for (auto& item : machine->Equipment)
        item.Type = -1;
    for (const auto slot : {EQUIPMENT_PANTS, EQUIPMENT_GLOVES, EQUIPMENT_BOOTS})
    {
        auto& item = machine->Equipment[slot];
        const int group = slot == EQUIPMENT_PANTS ? 9 : slot == EQUIPMENT_GLOVES ? 10 : 11;
        item.Type = group * itemGroupSize + 11;
        item.AncientDiscriminator = slot == EQUIPMENT_BOOTS ? 1 : 2;
        item.Durability = 1;
        ItemAttribute[item.Type].RequireClass[CLASS_ELF] = 1;
    }
    g_csItemOption.CheckItemSetOptions();
    CHECK(CharacterAttribute->AddStrength == 100);
    CHECK(CharacterAttribute->AddDexterity == 100);
    CHECK(CharacterAttribute->AddEnergy == 100);
    const auto fullVitality = CharacterAttribute->AddVitality;
    machine->Equipment[EQUIPMENT_BOOTS].Durability = 0;
    g_csItemOption.CheckItemSetOptions();
    CHECK(CharacterAttribute->AddStrength == 0);
    CHECK(CharacterAttribute->AddDexterity == 0);
    CHECK(CharacterAttribute->AddEnergy == 0);
    CHECK(fullVitality >= CharacterAttribute->AddVitality + 100);
    CharacterMachine = previousMachine;
    CharacterAttribute = previousAttribute;
    Hero = previousHero;
    ItemAttribute = previousItems;
}
void CheckEquippedArgoStats()
{
    constexpr int itemGroupSize = 512;
    // Exercise the same equipment recalculation that feeds the character window.
    auto machine = std::make_unique<CHARACTER_MACHINE>();
    auto hero = std::make_unique<CHARACTER>();
    auto itemAttributes = std::make_unique<ITEM_ATTRIBUTE[]>(MAX_ITEM);
    auto* previousMachine = CharacterMachine;
    auto* previousAttribute = CharacterAttribute;
    auto* previousHero = Hero;
    auto* previousItems = ItemAttribute;
    CharacterMachine = machine.get();
    CharacterAttribute = &machine->Character;
    Hero = hero.get();
    ItemAttribute = itemAttributes.get();
    Hero->Class = CLASS_ELF;
    for (auto& item : machine->Equipment)
        item.Type = -1;
    for (const auto slot : {EQUIPMENT_PANTS, EQUIPMENT_GLOVES, EQUIPMENT_ARMOR})
    {
        auto& item = machine->Equipment[slot];
        const int group = slot == EQUIPMENT_PANTS ? 9 : slot == EQUIPMENT_GLOVES ? 10 : 8;
        item.Type = group * itemGroupSize + 13;
        item.AncientDiscriminator = 1;
        item.Durability = 1;
        ItemAttribute[item.Type].RequireClass[CLASS_ELF] = 1;
    }
    g_csItemOption.CheckItemSetOptions();
    CHECK(CharacterAttribute->AddStrength == 50);
    CHECK(CharacterAttribute->AddDexterity == 50);
    CHECK(CharacterAttribute->AddEnergy == 50);
    CHECK(CharacterAttribute->AddCharisma == 0);
    const auto fullVitality = CharacterAttribute->AddVitality;
    machine->Equipment[EQUIPMENT_GLOVES].Durability = 0;
    g_csItemOption.CheckItemSetOptions();
    CHECK(CharacterAttribute->AddStrength == 0);
    CHECK(CharacterAttribute->AddDexterity == 0);
    CHECK(CharacterAttribute->AddEnergy == 0);
    CHECK(fullVitality >= CharacterAttribute->AddVitality + 50);
    CharacterMachine = previousMachine;
    CharacterAttribute = previousAttribute;
    Hero = previousHero;
    ItemAttribute = previousItems;
}
} // namespace

TEST_CASE("Tier 1 and Tier 2 Ancient tooltip previews and equipped character stats agree")
{
    const auto original = std::filesystem::current_path();
    std::filesystem::current_path(MU_ANCIENT_TEST_ASSETS);
    g_strSelectedML = L"Eng";
    const bool loaded = g_csItemOption.OpenItemSetScript();
    std::filesystem::current_path(original);
    REQUIRE(loaded);
    CheckAncientSetNames();
    REQUIRE_FALSE(g_csItemOption.IsAncientSetEquipped());

    struct Sample
    {
        int type;
        int variant;
        int bonus;
    };
    constexpr int itemGroupSize = 512;
    const std::array<Sample, 10> samples = {{{8 * itemGroupSize, 1, 100},
                                             {7 * itemGroupSize + 2, 2, 100},
                                             {9 * itemGroupSize + 11, 2, 100},
                                             {7 * itemGroupSize + 8, 1, 75},
                                             {8 * itemGroupSize + 6, 1, 50},
                                             {8 * itemGroupSize + 8, 1, 50},
                                             {8 * itemGroupSize + 7, 1, 50},
                                             {8 * itemGroupSize + 13, 1, 50},
                                             {8 * itemGroupSize + 40, 2, 50},
                                             {8 * itemGroupSize + 59, 2, 50}}};
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
    CheckEquippedFaseStats();
    CheckEquippedArgoStats();
}
