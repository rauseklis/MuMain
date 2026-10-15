#pragma once

namespace GameLogic::Skills
{
constexpr int ResolveSkillLevelRequirement(int configuredLevel, bool isSummonerSpell) noexcept
{
    return isSummonerSpell ? 0 : configuredLevel;
}

int RequiredSkillLevel(int skillType);
}
