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

}
