// NewUIBuffWindow.cpp: implementation of the CNewUIBuffWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "I18N/All.h"

#include "UI/NewUI/HUD/NewUIBuffWindow.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Textures/ZzzTexture.h"
#include "Engine/Object/ZzzInventory.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "GameLogic/Items/TwoHandedWeaponBonus.h"
#include "GameLogic/Buffs/BuffDetails.h"
#include "Network/Server/BuffDetailsProtocol.h"
#include "UI/NewUI/HUD/BuffTooltip.h"
#include "UI/NewUI/HUD/BuffDuration.h"
#include "Core/Text/Utf8.h"

using namespace SEASON3B;

namespace
{
    // One cell of the icon sheets, in texels of the original art.
    const float BUFF_SOURCE_WIDTH = 20.0f;
    const float BUFF_SOURCE_HEIGHT = 28.0f;
    const float BUFF_SHEET_SIZE = 256.0f;
    // Size on the 640 x 480 canvas: three quarters of the artwork, so the row stays small.
    const float BUFF_IMG_WIDTH = 15.0f;
    const float BUFF_IMG_HEIGHT = 21.0f;
    const int BUFF_MAX_LINE_COUNT = 8;
    const int BUFF_IMG_SPACE = 3;
    const int BUFF_ROW_WIDTH = BUFF_MAX_LINE_COUNT * static_cast<int>(BUFF_IMG_WIDTH) + (BUFF_MAX_LINE_COUNT - 1) * BUFF_IMG_SPACE;
    const int BUFF_TOP = 15;
    constexpr wchar_t TwoHandedBonusTitle[] = L"Two-handed weapon bonus";
    constexpr wchar_t PhysicalBonusText[] = L"Physical damage +15%";
    constexpr wchar_t WizardryBonusText[] = L"Wizardry damage +15%";
    constexpr wchar_t TwoHandedBonusCondition[] = L"Active while a two-handed weapon is equipped";
    constexpr wchar_t TwoHandedBonusLabel[] = L"2H";
    constexpr int TwoHandedTooltipLines = 4;
};

SEASON3B::CNewUIBuffWindow::CNewUIBuffWindow()
{
    m_pNewUIMng = NULL;
    m_Pos.x = m_Pos.y = 0;
}

SEASON3B::CNewUIBuffWindow::~CNewUIBuffWindow()
{
    Release();
}

bool SEASON3B::CNewUIBuffWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_BUFF_WINDOW, this);

    SetPos(x, y);

    LoadImages();

    Show(true);

    return true;
}

void SEASON3B::CNewUIBuffWindow::Release()
{
    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void SEASON3B::CNewUIBuffWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
}

void SEASON3B::CNewUIBuffWindow::SetPos(int /*iScreenWidth*/)
{
    // The row stays centred on the screen. It is narrower than the gap between two open panels, so it never has to
    // move aside when the inventory, character or another window opens.
    SetPos((REFERENCE_WIDTH - BUFF_ROW_WIDTH) / 2, BUFF_TOP);
}

static eBuffState NormalizeBuffState(eBuffState raw)
{
    switch (raw)
    {
    case EFFECT_GREATER_LIFE_ENHANCED:
    case EFFECT_GREATER_LIFE_MASTERED:
        return eBuff_Life;
    case EFFECT_MAGIC_CIRCLE_IMPROVED:
    case EFFECT_MAGIC_CIRCLE_ENHANCED:
        return eBuff_SwellOfMagicPower;
    case EFFECT_GREATER_CRITICAL_DAMAGE_MASTERED:
    case EFFECT_GREATER_CRITICAL_DAMAGE_EXTENDED:
        return eBuff_AddCriticalDamage;
    case EFFECT_INFINITY_ARROW_IMPROVED:
        return eBuff_InfinityArrow;
    case EFFECT_BLIND_IMPROVED:
        return eDeBuff_Blind;
    case EFFECT_POISON_ARROW_IMPROVED:
        return EFFECT_POISON_ARROW;
    case EFFECT_BLESS_IMPROVED:
        return EFFECT_BLESS;
    case EFFECT_IRON_DEFENSE_IMPROVED:
        return EFFECT_IRON_DEFENSE;
    case EFFECT_BLOOD_HOWLING_IMPROVED:
        return EFFECT_BLOOD_HOWLING;
    case EFFECT_GREATER_DEFENSE_SUCCESS_RATE_IMPROVED:
    case EFFECT_GREATER_DEFENSE_SUCCESS_RATE_ENHANCED:
        return eBuff_Def_up_Ourforces;
    case EFFECT_FITNESS_IMPROVED:
        return eBuff_Hp_up_Ourforces;
    case EFFECT_MANA_SHIELD_MASTERED:
        return eBuff_WizDefense;
    default:
        return raw;
    }
}

static int BuffTier(eBuffState buf)
{
    switch (buf)
    {
    case EFFECT_GREATER_LIFE_ENHANCED:
    case EFFECT_MAGIC_CIRCLE_IMPROVED:
    case EFFECT_GREATER_CRITICAL_DAMAGE_EXTENDED:
    case EFFECT_INFINITY_ARROW_IMPROVED:
    case EFFECT_BLIND_IMPROVED:
    case EFFECT_POISON_ARROW_IMPROVED:
    case EFFECT_BLESS_IMPROVED:
    case EFFECT_IRON_DEFENSE_IMPROVED:
    case EFFECT_BLOOD_HOWLING_IMPROVED:
    case EFFECT_GREATER_DEFENSE_SUCCESS_RATE_IMPROVED:
    case EFFECT_FITNESS_IMPROVED:
    case EFFECT_MANA_SHIELD_MASTERED:
        return 1;
    case EFFECT_GREATER_LIFE_MASTERED:
    case EFFECT_MAGIC_CIRCLE_ENHANCED:
    case EFFECT_GREATER_CRITICAL_DAMAGE_MASTERED:
    case EFFECT_GREATER_DEFENSE_SUCCESS_RATE_ENHANCED:
        return 2;
    default:
        return 0;
    }
}

void SEASON3B::CNewUIBuffWindow::BuffSort(std::list<eBuffState>& buffstate)
{
    OBJECT* pHeroObject = &Hero->Object;
    int iBuffSize = g_CharacterBuffSize(pHeroObject);

    eBuffState top[eBuff_Count] = {};

    for (int i = 0; i < iBuffSize; ++i)
    {
        eBuffState buf = g_CharacterBuff(pHeroObject, i);
        if (buf == eBuffNone || buf >= eBuff_Count ||
            GameLogic::Buffs::ActiveBuffDetails().IsExpired(buf, GameLogic::Buffs::BuffDetailsClock()))
        {
            continue;
        }
        if (SetDisableRenderBuff(buf))
        {
            continue;
        }

        eBuffState base = NormalizeBuffState(buf);
        if (top[base] == eBuffNone || BuffTier(buf) > BuffTier(top[base]))
        {
            top[base] = buf;
        }
    }

    for (int i = 0; i < iBuffSize; ++i)
    {
        eBuffState buf = g_CharacterBuff(pHeroObject, i);
        if (buf == eBuffNone || buf >= eBuff_Count ||
            GameLogic::Buffs::ActiveBuffDetails().IsExpired(buf, GameLogic::Buffs::BuffDetailsClock()))
        {
            continue;
        }
        if (SetDisableRenderBuff(buf))
        {
            continue;
        }

        eBuffState base = NormalizeBuffState(buf);
        if (buf != top[base])
        {
            continue;
        }

        eBuffClass eBuffClassType = g_IsBuffClass(buf);
        if (eBuffClassType == eBuffClass_Buff)
        {
            buffstate.push_front(buf);
        }
        else if (eBuffClassType == eBuffClass_DeBuff)
        {
            buffstate.push_back(buf);
        }
        else
        {
            continue;
        }
    }
}

bool SEASON3B::CNewUIBuffWindow::SetDisableRenderBuff(const eBuffState& _BuffState)
{
    switch (_BuffState)
    {
#ifdef PBG_ADD_PKSYSTEM_INGAMESHOP
    case eDeBuff_MoveCommandWin:
#endif //PBG_ADD_PKSYSTEM_INGAMESHOP
    case eDeBuff_FlameStrikeDamage:
    case eDeBuff_GiganticStormDamage:
    case eDeBuff_LightningShockDamage:
    case eDeBuff_Discharge_Stamina:
        return true;
    default:
        return false;
    }
    return false;
}

bool SEASON3B::CNewUIBuffWindow::UpdateMouseEvent()
{
    float x = 0.0f, y = 0.0f;
    int buffwidthcount = 0, buffheightcount = 0;

    std::list<eBuffState> buffstate;
    BuffSort(buffstate);

    std::list<eBuffState>::iterator iter;
    for (iter = buffstate.begin(); iter != buffstate.end(); )
    {
        auto tempiter = iter;
        ++iter;
        eBuffState buff = (*tempiter);

        const POINT position = GetBuffPosition(buffheightcount * BUFF_MAX_LINE_COUNT + buffwidthcount);
        x = position.x;
        y = position.y;

        if (SEASON3B::CheckMouseIn(x, y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT))
        {
            Network::Buffs::RequestDetails();
            if (buff == eBuff_InfinityArrow)
            {
                if (SEASON3B::IsRelease(VK_RBUTTON))
                {
                    SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CInfinityArrowCancelMsgBoxLayout));
                }
            }
            else if (buff == eBuff_SwellOfMagicPower)
            {
                if (SEASON3B::IsRelease(VK_RBUTTON))
                {
                    SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBuffSwellOfMPCancelMsgBoxLayOut));
                }
            }

            return false;
        }

        if (++buffwidthcount >= BUFF_MAX_LINE_COUNT) {
            buffwidthcount = 0;
            ++buffheightcount;
        }
    }
    if (HasTwoHandedBonus())
    {
        const POINT position = GetBuffPosition(static_cast<int>(buffstate.size()));
        if (SEASON3B::CheckMouseIn(position.x, position.y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT))
        {
            return false;
        }
    }
    return true;
}

bool SEASON3B::CNewUIBuffWindow::UpdateKeyEvent()
{
    return true;
}

bool SEASON3B::CNewUIBuffWindow::Update()
{
    const auto now = GameLogic::Buffs::BuffDetailsClock();
    OBJECT* heroObject = &Hero->Object;
    for (int index = 0; index < g_CharacterBuffSize(heroObject); ++index)
    {
        if (GameLogic::Buffs::ActiveBuffDetails().IsExpired(g_CharacterBuff(heroObject, index), now))
        {
            Network::Buffs::RequestDetails();
            break;
        }
    }
    return true;
}

bool SEASON3B::CNewUIBuffWindow::Render()
{
    EnableAlphaTest();

    RenderBuffStatus(BUFF_RENDER_ICON);

    RenderBuffStatus(BUFF_RENDER_TOOLTIP);

    DisableAlphaBlend();

    return true;
}

void SEASON3B::CNewUIBuffWindow::RenderBuffStatus(BUFF_RENDER renderstate)
{
    OBJECT* pHeroObject = &Hero->Object;

    float x = 0.0f, y = 0.0f;
    int buffwidthcount = 0, buffheightcount = 0;

    std::list<eBuffState> buffstate;
    BuffSort(buffstate);

    std::list<eBuffState>::iterator iter;
    for (iter = buffstate.begin(); iter != buffstate.end(); )
    {
        auto tempiter = iter;
        ++iter;
        eBuffState buff = (*tempiter);

        const POINT position = GetBuffPosition(buffheightcount * BUFF_MAX_LINE_COUNT + buffwidthcount);
        x = position.x;
        y = position.y;

        if (renderstate == BUFF_RENDER_ICON)
        {
            RenderBuffIcon(buff, x, y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT);
#ifdef _DEBUG
            int iBuffReferenceCount = g_CharacterBuffCount(pHeroObject, buff);
            RenderNumber(x + 5, y + 5, iBuffReferenceCount, 1.f);
#endif //_DEBUG
        }
        else if (renderstate == BUFF_RENDER_TOOLTIP)
        {
            // ���� ���� ������
            if (SEASON3B::CheckMouseIn(x, y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT)) {
                float fTooltip_x = x + (BUFF_IMG_WIDTH / 2);
                float fTooltip_y = y + BUFF_IMG_WIDTH;
                eBuffClass buffclass = g_IsBuffClass(buff);
                RenderBuffTooltip(buffclass, buff, fTooltip_x, fTooltip_y);
            }
        }

        if (++buffwidthcount >= BUFF_MAX_LINE_COUNT) {
            buffwidthcount = 0;
            ++buffheightcount;
        }
    }
    RenderTwoHandedBonus(renderstate, static_cast<int>(buffstate.size()));
}

bool SEASON3B::CNewUIBuffWindow::HasTwoHandedBonus() const
{
    if (!CharacterMachine)
    {
        return false;
    }

    const ITEM& rightWeapon = CharacterMachine->Equipment[EQUIPMENT_WEAPON_RIGHT];
    const ITEM& leftWeapon = CharacterMachine->Equipment[EQUIPMENT_WEAPON_LEFT];
    if (rightWeapon.Type < ITEM_SWORD || rightWeapon.Type >= ITEM_SHIELD)
    {
        return false;
    }

    return GameLogic::Items::HasTwoHandedWeaponBonus(
        rightWeapon.Type, ItemAttribute[rightWeapon.Type].TwoHand, leftWeapon.Type);
}

POINT SEASON3B::CNewUIBuffWindow::GetBuffPosition(int index) const
{
    return {
        m_Pos.x + (index % BUFF_MAX_LINE_COUNT) * static_cast<int>(BUFF_IMG_WIDTH + BUFF_IMG_SPACE),
        m_Pos.y + (index / BUFF_MAX_LINE_COUNT) * static_cast<int>(BUFF_IMG_HEIGHT + BUFF_IMG_SPACE)
    };
}

void SEASON3B::CNewUIBuffWindow::RenderTwoHandedBonus(BUFF_RENDER renderstate, int index)
{
    if (!HasTwoHandedBonus())
    {
        return;
    }

    const POINT position = GetBuffPosition(index);
    if (renderstate == BUFF_RENDER_ICON)
    {
        eBuffState artwork = eBuff_Scroll_Strengthen;
        RenderBuffIcon(artwork, position.x, position.y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT);
        const DWORD textColor = g_pRenderText->GetTextColor();
        const DWORD backgroundColor = g_pRenderText->GetBgColor();
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 180);
        g_pRenderText->RenderText(position.x, position.y + BUFF_IMG_HEIGHT / 2,
            TwoHandedBonusLabel, BUFF_IMG_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->SetTextColor(textColor);
        g_pRenderText->SetBgColor(backgroundColor);
        g_pRenderText->SetFont(g_hFont);
        return;
    }

    if (!SEASON3B::CheckMouseIn(position.x, position.y, BUFF_IMG_WIDTH, BUFF_IMG_HEIGHT))
    {
        return;
    }

    const wchar_t* lines[TwoHandedTooltipLines] = {
        TwoHandedBonusTitle, PhysicalBonusText, WizardryBonusText, TwoHandedBonusCondition
    };
    int count = 0;
    for (int line = 0; line < TwoHandedTooltipLines; ++line)
    {
        UI::Buffs::Tooltip::Append(lines[line], line == 0 ? TEXT_COLOR_BLUE : TEXT_COLOR_WHITE, line == 0, count);
        if (line == 0)
        {
            UI::Buffs::Tooltip::AppendDivider(count);
        }
    }
    UI::Buffs::Tooltip::Render(position.x + BUFF_IMG_WIDTH / 2,
        position.y + BUFF_IMG_HEIGHT + BUFF_IMG_SPACE, count, IMAGE_BUFF_TOOLTIP_ORNAMENTS);
}

void SEASON3B::CNewUIBuffWindow::RenderBuffIcon(eBuffState& eBuffType, float x, float y, float width, float height)
{
    int iWidthIndex, iHeightIndex;
    float u, v;

    if (eBuffType < 81) // eBuff_Berserker
    {
        iWidthIndex = (eBuffType - 1) % 10;
        iHeightIndex = (eBuffType - 1) / 10;

        u = iWidthIndex * BUFF_SOURCE_WIDTH / BUFF_SHEET_SIZE;
        v = iHeightIndex * BUFF_SOURCE_HEIGHT / BUFF_SHEET_SIZE;

        RenderBitmap(IMAGE_BUFF_STATUS, x, y, width, height, u, v, BUFF_SOURCE_WIDTH / BUFF_SHEET_SIZE, BUFF_SOURCE_HEIGHT / BUFF_SHEET_SIZE);
    }
    else if (eBuffType < 161)
    {
        // eBuff_Santa
        iWidthIndex = (eBuffType - 81) % 10; // eBuff_Berserker
        iHeightIndex = (eBuffType - 81) / 10; // eBuff_Berserker

        u = iWidthIndex * BUFF_SOURCE_WIDTH / BUFF_SHEET_SIZE;
        v = iHeightIndex * BUFF_SOURCE_HEIGHT / BUFF_SHEET_SIZE;

        RenderBitmap(IMAGE_BUFF_STATUS2, x, y, width, height, u, v, BUFF_SOURCE_WIDTH / BUFF_SHEET_SIZE, BUFF_SOURCE_HEIGHT / BUFF_SHEET_SIZE);
    }
    else
    {
        const int index = eBuffType - 161;
        u = (index % 10) * BUFF_SOURCE_WIDTH / BUFF_SHEET_SIZE;
        v = (index / 10) * BUFF_SOURCE_HEIGHT / BUFF_SHEET_SIZE;
        RenderBitmap(IMAGE_BUFF_STATUS3, x, y, width, height, u, v, BUFF_SOURCE_WIDTH / BUFF_SHEET_SIZE, BUFF_SOURCE_HEIGHT / BUFF_SHEET_SIZE);
    }
}

void SEASON3B::CNewUIBuffWindow::RenderBuffTooltip(eBuffClass& eBuffClassType, eBuffState& eBuffType, float x, float y)
{
    int count = 0;
    auto info = g_BuffInfo(eBuffType);
    if (info.s_BuffName[0] == L'\0')
    {
        info = g_BuffInfo(NormalizeBuffState(eBuffType));
    }
    UI::Buffs::Tooltip::Append(info.s_BuffName, TEXT_COLOR_BLUE, true, count);
    UI::Buffs::Tooltip::AppendDivider(count);
    const auto* details = GameLogic::Buffs::ActiveBuffDetails().Find(eBuffType);
    if (details)
    {
        for (const auto& line : details->lines)
        {
            UI::Buffs::Tooltip::Append(Core::Text::FromUtf8(line.text),
                line.penalty ? TEXT_COLOR_RED : TEXT_COLOR_WHITE, false, count);
        }
        if (details->lines.empty())
        {
            for (const auto& line : info.s_BuffDescriptlist)
            {
                UI::Buffs::Tooltip::Append(line, TEXT_COLOR_WHITE, false, count);
            }
        }
        UI::Buffs::Tooltip::Append(UI::Buffs::FormatDuration(details->expiresAt, GameLogic::Buffs::BuffDetailsClock()),
            TEXT_COLOR_PURPLE, false, count);
    }
    else
    {
        for (const auto& line : info.s_BuffDescriptlist)
        {
            UI::Buffs::Tooltip::Append(line, TEXT_COLOR_WHITE, false, count);
        }
        std::wstring bufftime;
        g_BuffStringTime(eBuffType, bufftime);
        UI::Buffs::Tooltip::Append(bufftime, TEXT_COLOR_PURPLE, false, count);
    }
    UI::Buffs::Tooltip::Render(x, y, count, IMAGE_BUFF_TOOLTIP_ORNAMENTS);
}

float SEASON3B::CNewUIBuffWindow::GetLayerDepth()	//. 5.3f
{
    return 0.95f;
}

void SEASON3B::CNewUIBuffWindow::OpenningProcess()
{
}

void SEASON3B::CNewUIBuffWindow::ClosingProcess()
{
}

void SEASON3B::CNewUIBuffWindow::LoadImages()
{
    LoadBitmap(L"Interface\\BuffDetails\\newui_statusicon.jpg", IMAGE_BUFF_STATUS, GL_LINEAR);
    LoadBitmap(L"Interface\\BuffDetails\\newui_statusicon2.jpg", IMAGE_BUFF_STATUS2, GL_LINEAR);
    LoadBitmap(L"Interface\\BuffDetails\\newui_statusicon3.jpg", IMAGE_BUFF_STATUS3, GL_LINEAR);
    LoadBitmap(L"Interface\\BuffDetails\\tooltip_ornaments.tga", IMAGE_BUFF_TOOLTIP_ORNAMENTS, GL_LINEAR);
}

void SEASON3B::CNewUIBuffWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_BUFF_TOOLTIP_ORNAMENTS);
    DeleteBitmap(IMAGE_BUFF_STATUS3);
    DeleteBitmap(IMAGE_BUFF_STATUS2);
    DeleteBitmap(IMAGE_BUFF_STATUS);
}
