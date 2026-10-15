#include "doctest.h"
#include "GameLogic/Skills/SkillLevelRequirements.h"
#include <initializer_list>

using GameLogic::Skills::ResolveSkillLevelRequirement;

TEST_CASE("Summoner spells remain available at reset level [skills][reset]")
{
    for (const int configuredLevel : {0, 1, 35, 75, 93, 111, 220, 380, 400})
    {
        CHECK(ResolveSkillLevelRequirement(configuredLevel, true) == 0);
    }
}

TEST_CASE("Other classes retain skill level requirements [skills][reset]")
{
    for (const int configuredLevel : {0, 1, 35, 75, 93, 111, 220, 380, 400})
    {
        CHECK(ResolveSkillLevelRequirement(configuredLevel, false) == configuredLevel);
    }
}
