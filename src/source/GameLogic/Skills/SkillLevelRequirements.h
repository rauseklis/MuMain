#pragma once

namespace GameLogic::Skills
{
constexpr int ResolveSkillLevelRequirement(int configuredLevel, bool isSummonerSpell) noexcept
{
    return isSummonerSpell ? 0 : configuredLevel;
}

int RequiredSkillLevel(int skillType);
int RequiredSkillScrollLevel(int itemType, int configuredLevel);
bool CanLearnSkillScroll(int itemType, int requiredStrength, int requiredEnergy);
}
