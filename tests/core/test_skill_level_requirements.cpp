#include "doctest.h"
#include "GameLogic/Skills/SkillLevelRequirements.h"
#include "GameLogic/Skills/SkillRequirementInfo.h"
#include <initializer_list>

using GameLogic::Skills::ResolveSkillLevelRequirement;

TEST_CASE("All classes can cast learned skills without a character level gate [skills][reset]")
{
    for (const int configuredLevel : {0, 1, 35, 75, 93, 111, 220, 380, 400})
    {
        CHECK(ResolveSkillLevelRequirement(configuredLevel) == 0);
    }
}

TEST_CASE("Casting still requires every stat at its exact threshold [skills][stats]")
{
    using StatMember = WORD DemendConditionInfo::*;
    for (const StatMember stat :
         {&DemendConditionInfo::SkillStrength, &DemendConditionInfo::SkillDexterity,
          &DemendConditionInfo::SkillVitality, &DemendConditionInfo::SkillEnergy, &DemendConditionInfo::SkillCharisma})
    {
        DemendConditionInfo required;
        required.SkillLevel = static_cast<WORD>(ResolveSkillLevelRequirement(400));
        required.*stat = 220;
        DemendConditionInfo character;
        character.SkillLevel = 1;
        character.*stat = 219;
        CHECK_FALSE(required <= character);
        character.*stat = 220;
        CHECK(required <= character);
        character.*stat = 221;
        CHECK(required <= character);
    }
}
