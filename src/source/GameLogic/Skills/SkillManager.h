// SkillManager.h: interface for the CSkillManager class.
//////////////////////////////////////////////////////////////////////
#pragma once

#include "Core/Globals/_enum.h"
#include "GameLogic/Skills/SkillRequirementInfo.h"

class CSkillManager
{
public:
    CSkillManager();
    virtual ~CSkillManager();
    bool FindHeroSkill(ActionSkillType eSkillType);
    void GetSkillInformation(int iType, int iLevel, wchar_t* lpszName, int* piMana, int* piDistance, int* piSkillMana = NULL);
    void GetSkillInformation_Energy(int iType, int* piEnergy);
    void GetSkillInformation_Charisma(int iType, int* piCharisma);
    float GetSkillDistance(int Index, CHARACTER* c = NULL);
    void GetSkillInformation_Damage(int iType, int* piDamage);
    bool CheckSkillDelay(int SkillIndex);
    void CalcSkillDelay(int time);
    BYTE GetSkillMasteryType(ActionSkillType iType);
    ActionSkillType MasterSkillToBaseSkillIndex(ActionSkillType masterSkill);
    bool skillVScharactorCheck(const DemendConditionInfo& basicInfo, const DemendConditionInfo& heroInfo);
    bool AreSkillAttributeRequirementsMet(ActionSkillType skilltype);

    void InvalidateSkillAttributeRequirementsCache();
    void InitializeSkillAttributeRequirementsCache();

private:
    DemendConditionInfo BuildSkillAttributeRequirements(ActionSkillType skillType);
    void RebuildSkillAttributeRequirementsCache();

    bool m_bSkillAttributeRequirementsCacheDirty;
    bool m_aSkillAttributeRequirementsMet[MAX_SKILLS];
};

extern CSkillManager gSkillManager;
