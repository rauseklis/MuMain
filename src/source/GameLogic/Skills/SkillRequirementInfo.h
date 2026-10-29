#pragma once

#include "Core/Platform/WinCompat.h"

typedef struct DemendConditionInfo
{
    WORD SkillType;
    wchar_t SkillName[100];
    WORD SkillLevel;
    WORD SkillStrength;
    WORD SkillDexterity;
    WORD SkillVitality;
    WORD SkillEnergy;
    WORD SkillCharisma;

    DemendConditionInfo()
        : SkillType(0), SkillLevel(0), SkillStrength(0), SkillDexterity(0), SkillVitality(0), SkillEnergy(0),
          SkillCharisma(0)
    {
        ZeroMemory(SkillName, 100);
    }
    BOOL operator<=(const DemendConditionInfo& rhs) const
    {
        return SkillLevel <= rhs.SkillLevel && SkillStrength <= rhs.SkillStrength &&
               SkillDexterity <= rhs.SkillDexterity && SkillVitality <= rhs.SkillVitality &&
               SkillEnergy <= rhs.SkillEnergy && SkillCharisma <= rhs.SkillCharisma;
    }
} DemendConditionInfo;
