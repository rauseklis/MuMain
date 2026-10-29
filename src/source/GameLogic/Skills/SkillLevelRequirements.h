#pragma once

namespace GameLogic::Skills
{
constexpr int ResolveSkillLevelRequirement(int /*configuredLevel*/) noexcept
{
    return 0;
}

int RequiredSkillLevel(int skillType);
} // namespace GameLogic::Skills
