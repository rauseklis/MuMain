#include "stdafx.h"
#include "GameLogic/Skills/SkillLevelRequirements.h"

#include "Engine/Object/ZzzInfomation.h"

namespace GameLogic::Skills
{
int RequiredSkillLevel(int skillType)
{
    const auto& skill = SkillAttribute[skillType];
    return ResolveSkillLevelRequirement(skill.Level);
}

} // namespace GameLogic::Skills
