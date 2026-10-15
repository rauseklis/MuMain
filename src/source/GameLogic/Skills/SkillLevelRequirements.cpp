#include "stdafx.h"
#include "GameLogic/Skills/SkillLevelRequirements.h"

#include "Character/CharacterManager.h"
#include "Engine/Object/ZzzInfomation.h"

namespace GameLogic::Skills
{
int RequiredSkillLevel(int skillType)
{
    const auto& skill = SkillAttribute[skillType];
    const bool isSummonerSpell = gCharacterManager.GetBaseClass(CharacterAttribute->Class) == CLASS_SUMMONER
                                 && skill.RequireClass[CLASS_SUMMONER] != 0;
    return ResolveSkillLevelRequirement(skill.Level, isSummonerSpell);
}

int RequiredSkillScrollLevel(int itemType, int configuredLevel)
{
    if (itemType < ITEM_ETC || itemType >= ITEM_ETC + MAX_ITEM_INDEX)
    {
        return configuredLevel;
    }

    const bool isSummonerSpell = gCharacterManager.GetBaseClass(CharacterAttribute->Class) == CLASS_SUMMONER
                                 && ItemAttribute[itemType].RequireClass[CLASS_SUMMONER] != 0;
    return ResolveSkillLevelRequirement(configuredLevel, isSummonerSpell);
}

bool CanLearnSkillScroll(int itemType, int requiredStrength, int requiredEnergy)
{
    const int requiredLevel = RequiredSkillScrollLevel(itemType, ItemAttribute[itemType].RequireLevel);
    const WORD strength = CharacterAttribute->Strength + CharacterAttribute->AddStrength;
    const WORD energy = CharacterAttribute->Energy + CharacterAttribute->AddEnergy;
    return CharacterAttribute->Level >= requiredLevel && strength >= requiredStrength && energy >= requiredEnergy;
}
}
