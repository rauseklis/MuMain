#include "stdafx.h"
#include "App/Platform/Windows/Winmain.h"
#include "Render/Textures/ZzzTexture.h"
#include "GameLogic/Items/CSItemOption.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/HUD/NewUIMasterLevel.h"
#include "UI/NewUI/HUD/BuffTooltip.h"
#include "UI/NewUI/HUD/NewUIBuffWindow.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "GameLogic/Skills/SkillManager.h"

namespace 
{
    _MASTER_SKILLTREE_DATA m_stMasterSkillTreeData[MAX_MASTER_SKILL_DATA];
    _MASTER_SKILL_TOOLTIP m_stMasterSkillTooltip[MAX_MASTER_SKILL_DATA];
}


namespace
{
    using Auction = SEASON3B::CNewUIAuctionWindow;
    namespace Tree = UI::MasterTree;

    constexpr int TitleY = 13;
    constexpr int InfoY = 40;
    constexpr int InfoHeight = 22;
    constexpr int InfoGap = 6;
    constexpr int InfoPadding = 14;
    constexpr int InfoMinimumWidth = 96;
    constexpr int CloseWidth = 36;
    constexpr int CloseHeight = 29;
    constexpr int CloseMargin = 8;
    constexpr int TooltipGap = 4;
    constexpr int TooltipFlipY = 240;
    constexpr int ExpTooltipLines = 1;
    constexpr int TooltipLineCapacity = 30;

    constexpr float LineThickness = 2.f;
    constexpr float ShadowOffset = 1.f;
    constexpr float HeadLength = 4.f;
    constexpr float HeadSpread = 3.f;
    constexpr float ConnectorInset = 1.f;
    constexpr float BadgeWidth = 17.f;
    constexpr float BadgeHeight = 13.f;
    constexpr float BadgeOverlap = 5.f;

    constexpr unsigned int ConnectorLit = 0xFFE8BE5Au;
    constexpr unsigned int ConnectorDim = 0xFF5C6270u;
    constexpr unsigned int ConnectorShadow = 0x90000000u;

    // Each tree has its own hue: Peace green, Wisdom blue, Overcome red. The tints multiply the steel chrome, so
    // they are muted tones of the hue; the header is brighter once points are spent in the tree.
    const DWORD ColumnTint[Tree::ColumnCount] = { RGBA(88, 124, 98, 255), RGBA(86, 108, 146, 255), RGBA(146, 90, 90, 255) };
    const DWORD HeaderTintActive[Tree::ColumnCount] = { RGBA(124, 224, 140, 255), RGBA(126, 178, 255, 255), RGBA(255, 126, 116, 255) };
    const DWORD HeaderTintIdle[Tree::ColumnCount] = { RGBA(86, 134, 98, 255), RGBA(88, 118, 168, 255), RGBA(158, 92, 88, 255) };
    const DWORD HeaderTextActive[Tree::ColumnCount] = { RGBA(204, 255, 212, 255), RGBA(206, 228, 255, 255), RGBA(255, 214, 208, 255) };
    const DWORD HeaderTextIdle[Tree::ColumnCount] = { RGBA(176, 214, 184, 255), RGBA(180, 202, 236, 255), RGBA(226, 184, 180, 255) };
    const DWORD SlotTintLearned = RGBA(232, 198, 112, 255);
    const DWORD SlotTintAvailable = RGBA(172, 178, 190, 255);
    const DWORD SlotTintLocked = RGBA(84, 88, 98, 255);
    const DWORD SlotTintHover = RGBA(236, 241, 250, 255);
    const DWORD BadgeTint = RGBA(70, 74, 84, 255);

    void DrawLine(float x1, float y1, float x2, float y2, unsigned int color)
    {
        RenderColorLineARGB(x1 + ShadowOffset, y1 + ShadowOffset, x2 + ShadowOffset, y2 + ShadowOffset, LineThickness, ConnectorShadow);
        RenderColorLineARGB(x1, y1, x2, y2, LineThickness, color);
    }

    // A chevron that points along (dx, dy), a unit axis direction, with its tip at (x, y).
    void DrawHead(float x, float y, float dx, float dy, unsigned int color)
    {
        const float backX = x - dx * HeadLength;
        const float backY = y - dy * HeadLength;
        DrawLine(backX - dy * HeadSpread, backY + dx * HeadSpread, x, y, color);
        DrawLine(backX + dy * HeadSpread, backY - dx * HeadSpread, x, y, color);
    }

    // The connector a skill draws towards the ones that depend on it (the arrow codes of the skill tree data):
    // 1 and 2 run to the neighbour on the right (the head at its end, or at this skill), 3 and 4 run down one or two
    // rows, 7 and 8 run down one row and across to the next column to the right or left.
    void RenderConnector(int direction, const Tree::Slot& slot, const Tree::RowMetrics& rows, unsigned int color)
    {
        const float cellStep = static_cast<float>(Tree::CellWidth);
        const float mx = slot.centreX();
        const float my = slot.centreY();
        switch (direction)
        {
        case 1:
        case 2:
        {
            const float from = slot.right() + ConnectorInset;
            const float to = slot.x + cellStep - ConnectorInset;
            DrawLine(from, my, to, my, color);
            if (direction == 1)
            {
                DrawHead(to, my, 1.f, 0.f, color);
            }
            else
            {
                DrawHead(from, my, -1.f, 0.f, color);
            }
            break;
        }
        case 3:
        case 4:
        {
            const float to = slot.y + rows.pitch * (direction == 3 ? 1.f : 2.f) - ConnectorInset;
            DrawLine(mx, slot.bottom() + ConnectorInset, mx, to, color);
            DrawHead(mx, to, 0.f, 1.f, color);
            break;
        }
        case 7:
        case 8:
        {
            const float side = direction == 7 ? 1.f : -1.f;
            const float bendY = slot.y + rows.pitch + slot.height / 2.f;
            const float landing = (direction == 7 ? slot.x + cellStep : slot.right() - cellStep) - side * ConnectorInset;
            DrawLine(mx, slot.bottom() + ConnectorInset, mx, bendY, color);
            DrawLine(mx, bendY, landing, bendY, color);
            DrawHead(landing, bendY, side, 0.f, color);
            break;
        }
        default:
            break;
        }
    }

    // Skills in a row are listed left to right, four to a row.
    int CellOf(const _MASTER_SKILLTREE_DATA& data)
    {
        return (data.Index - 1) % Tree::CellsPerRow;
    }
}


SEASON3B::CNewUIMasterLevel::CNewUIMasterLevel()
{
    m_pNewUIMng = nullptr;
    this->ConsumePoint = 0;
    this->CurSkillID = 0;
    this->classCode = MASTER_SKILL_TREE_CLASS_NONE;
    this->CategoryTextIndex = 0;
    this->InitMasterSkillPoint();
    this->ClearSkillTreeData();
    this->ClearSkillTooltipData();
}

SEASON3B::CNewUIMasterLevel::~CNewUIMasterLevel()
{
    this->Release();
}

BYTE SEASON3B::CNewUIMasterLevel::GetConsumePoint() const
{
    return this->ConsumePoint;
}

int SEASON3B::CNewUIMasterLevel::GetCurSkillID() const
{
    return this->CurSkillID;
}

bool SEASON3B::CNewUIMasterLevel::Create(CNewUIManager* pNewUIMng)
{
    if (nullptr == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_MASTER_LEVEL, this);

    this->SetPos();

    this->LoadImages();

    return true;
}

void SEASON3B::CNewUIMasterLevel::Release()
{
    this->ClearSkillTreeData();
    this->ClearSkillTooltipData();
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIMasterLevel::SetPos()
{
    this->PosX = 0;
    this->PosY = 0;
    this->width = UI::MasterTree::WindowWidth;
    this->height = UI::MasterTree::WindowHeight;
}

void SEASON3B::CNewUIMasterLevel::OpenMasterSkillTreeData(const wchar_t* path)
{
    memset(m_stMasterSkillTreeData, 0, sizeof(m_stMasterSkillTreeData));

    FILE* fp = _wfopen(path, L"rb");

    wchar_t Text[256];

    if (fp == nullptr)
    {
        mu_swprintf(Text, L"%ls - File not exist.", path);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }

    constexpr int Size = sizeof(_MASTER_SKILLTREE_DATA);

    auto Buffer = new BYTE[Size * MAX_MASTER_SKILL_DATA];

    fread(Buffer, Size * MAX_MASTER_SKILL_DATA, 1, fp);

    DWORD dwCheckSum;

    fread(&dwCheckSum, sizeof(DWORD), 1u, fp);

    fclose(fp);

    if (dwCheckSum != GenerateCheckSum2(Buffer, 12288, 0x2BC1))
    {
        mu_swprintf(Text, L"%ls - File corrupted.", path);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }

    BYTE* pSeek = Buffer;

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        BuxConvert(pSeek, Size);

        memcpy(&m_stMasterSkillTreeData[i], pSeek, Size);

        pSeek += Size;

        if (pSeek == nullptr)
        {
            break;
        }
    }

    delete[] Buffer;
}

void SEASON3B::CNewUIMasterLevel::OpenMasterSkillTooltip(const wchar_t* path)
{
    memset(m_stMasterSkillTooltip, 0, sizeof(m_stMasterSkillTooltip));

    FILE* fp = _wfopen(path, L"rb");

    if (fp == nullptr)
    {
        wchar_t Text[256];
        mu_swprintf(Text, L"%ls - File not exist.", path);
        g_ErrorReport.Write(Text);
        MessageBox(g_hWnd, Text, nullptr, MB_OK);
        SendMessage(g_hWnd, WM_DESTROY, 0, 0);
        return;
    }

    constexpr int record_size = sizeof(_MASTER_SKILL_TOOLTIP_FILE);
    auto file_buffer = new BYTE[record_size * MAX_MASTER_SKILL_DATA];
    fread(file_buffer, record_size * MAX_MASTER_SKILL_DATA, 1, fp);
    DWORD dwCheckSum;
    fread(&dwCheckSum, sizeof(DWORD), 1u, fp);
    fclose(fp);

    BYTE* pSeek = file_buffer;

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        BuxConvert(pSeek, record_size);

        _MASTER_SKILL_TOOLTIP_FILE current{ };
        memcpy(&current, pSeek, record_size);

        const auto target = &m_stMasterSkillTooltip[i];
        target->SkillNumber = static_cast<ActionSkillType>(current.SkillNumber);
        target->ClassCode = static_cast<MASTER_SKILL_TREE_CLASS>(current.ClassCode);
        CMultiLanguage::ConvertFromUtf8(target->Info1, current.Info1);
        CMultiLanguage::ConvertFromUtf8(target->Info2, current.Info2);
        CMultiLanguage::ConvertFromUtf8(target->Info3, current.Info3);
        CMultiLanguage::ConvertFromUtf8(target->Info4, current.Info4);
        CMultiLanguage::ConvertFromUtf8(target->Info5, current.Info5);
        CMultiLanguage::ConvertFromUtf8(target->Info6, current.Info6);
        CMultiLanguage::ConvertFromUtf8(target->Info7, current.Info7);

        pSeek += record_size;

        if (pSeek == nullptr)
        {
            break;
        }
    }

    delete[] file_buffer;
}

void SEASON3B::CNewUIMasterLevel::InitMasterSkillPoint()
{
    for (int i = 0; i < 3; i++)
    {
        this->CategoryPoint[i] = 0;
        for (int k = 0; k < 10; k++)
        {
            this->skillPoint[i][k] = 0;
        }
    }
}

void SEASON3B::CNewUIMasterLevel::SetMasterType(CLASS_TYPE Class)
{
    switch (Class)
    {
    case CLASS_GRANDMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_GRANDMASTER;
        break;
    case CLASS_BLADEMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_BLADEMASTER;
        break;
    case CLASS_HIGHELF:
        this->classCode = MASTER_SKILL_TREE_CLASS_HIGHELF;
        break;
    case CLASS_DUELMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_DUELMASTER;
        break;
    case CLASS_LORDEMPEROR:
        this->classCode = MASTER_SKILL_TREE_CLASS_LORDEMPEROR;
        break;
    case CLASS_DIMENSIONMASTER:
        this->classCode = MASTER_SKILL_TREE_CLASS_DIMENSIONMASTER;
        break;
    case CLASS_TEMPLENIGHT:
        this->classCode = MASTER_SKILL_TREE_CLASS_TEMPLEKNIGHT;
        break;
    default:
        break;
    }

    this->SetMasterSkillTreeData();

    this->SetMasterSkillToolTipData();

    this->RecalculateRows();

    switch (Class)
    {
    case CLASS_WIZARD:
    case CLASS_SOULMASTER:
    case CLASS_GRANDMASTER:
        this->CategoryTextIndex = 1751;
        this->ClassNameTextIndex = 1669;
        break;
    case CLASS_KNIGHT:
    case CLASS_BLADEKNIGHT:
    case CLASS_BLADEMASTER:
        this->CategoryTextIndex = 1755;
        this->ClassNameTextIndex = 1668;
        break;
    case CLASS_ELF:
    case CLASS_MUSEELF:
    case CLASS_HIGHELF:
        this->CategoryTextIndex = 1759;
        this->ClassNameTextIndex = 1670;
        break;
    case CLASS_DARK:
    case CLASS_DUELMASTER:
        this->CategoryTextIndex = 1763;
        this->ClassNameTextIndex = 1671;
        break;
    case CLASS_DARK_LORD:
    case CLASS_LORDEMPEROR:
        this->CategoryTextIndex = 1767;
        this->ClassNameTextIndex = 1672;
        break;
    case CLASS_SUMMONER:
    case CLASS_BLOODYSUMMONER:
    case CLASS_DIMENSIONMASTER:
        this->CategoryTextIndex = 3136;
        this->ClassNameTextIndex = 1689;
        break;
    case CLASS_RAGEFIGHTER:
    case CLASS_TEMPLENIGHT:
        this->CategoryTextIndex = 3330;
        this->ClassNameTextIndex = 3151;
        break;
    default:
        return;
    }
}

void SEASON3B::CNewUIMasterLevel::SetMasterSkillTreeData()
{
    this->ClearSkillTreeData();

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        if (m_stMasterSkillTreeData[i].Index == 0)
        {
            break;
        }

        if ((this->classCode & m_stMasterSkillTreeData[i].ClassCode) == 0)
        {
            continue;
        }

        if (!this->map_masterData.insert(std::pair<BYTE, _MASTER_SKILLTREE_DATA>(m_stMasterSkillTreeData[i].Index, m_stMasterSkillTreeData[i])).second)
        {
            break;
        }
    }
}

void SEASON3B::CNewUIMasterLevel::SetMasterSkillToolTipData()
{
    this->ClearSkillTooltipData();

    for (int i = 0; i < MAX_MASTER_SKILL_DATA; i++)
    {
        if (m_stMasterSkillTooltip[i].SkillNumber == 0)
        {
            break;
        }

        if ((this->classCode & m_stMasterSkillTooltip[i].ClassCode) == 0)
        {
            continue;
        }

        if (!this->map_masterSkillToolTip.insert(std::pair(m_stMasterSkillTooltip[i].SkillNumber, m_stMasterSkillTooltip[i])).second)
        {
            break;
        }
    }
}

bool SEASON3B::CNewUIMasterLevel::SetMasterSkillTreeInfo(int index, BYTE skillLevel, float value, float nextvalue)
{
    const auto it = this->map_masterData.find(index);

    if (it == this->map_masterData.end())
    {
        return false;
    }

    const CSkillTreeInfo skillInfo = { skillLevel, value, nextvalue };
    CharacterAttribute->MasterSkillInfo[it->second.Skill] = skillInfo;

    this->CategoryPoint[it->second.Group] += skillLevel;

    return true;
}

int SEASON3B::CNewUIMasterLevel::SetDivideString(wchar_t* text, int isItemTollTip, int TextNum, int iTextColor, int iTextBold, bool isPercent)
{
    if (text == nullptr)
    {
        return TextNum;
    }

    constexpr wchar_t alpszDst[10][256] = {};

    int  nLine = 0;

    if (isItemTollTip == 0)
    {
        nLine = DivideStringByPixel((LPTSTR)alpszDst, 10, 256, text, 150, true, 35);
    }
    else if (isItemTollTip == 1)
    {
        nLine = DivideStringByPixel((LPTSTR)alpszDst, 10, 256, text, 200, true, 35);
    }

    for (int i = 0; i < nLine; i++)
    {
        TextListColor[TextNum] = iTextColor;

        TextBold[TextNum] = iTextBold;

        std::wstring cText = alpszDst[i];

        if (isPercent)
        {
            for (int j = cText.find(L"%", 0); j != -1; j = cText.find(L"%", j + 2))
            {
                cText.insert(j, L"%");
            }
        }

        mu_swprintf(TextList[TextNum], cText.c_str());

        TextNum++;
    }

    return TextNum;
}

int SEASON3B::CNewUIMasterLevel::HighestRank() const
{
    int highest = 1;
    for (const auto& entry : this->map_masterData)
    {
        highest = std::max<int>(highest, SkillAttribute[entry.second.Skill].SkillRank);
    }
    return highest;
}

void SEASON3B::CNewUIMasterLevel::RecalculateRows()
{
    this->m_rows = UI::MasterTree::ComputeRows(this->HighestRank(),
        static_cast<float>(UI::MasterTree::CellsBottom - UI::MasterTree::CellsTop));
}

UI::MasterTree::Slot SEASON3B::CNewUIMasterLevel::SlotOf(const _MASTER_SKILLTREE_DATA& data) const
{
    auto slot = UI::MasterTree::SlotOf(data.Group, CellOf(data), SkillAttribute[data.Skill].SkillRank, this->m_rows);
    slot.x += this->PosX;
    slot.y += this->PosY;
    return slot;
}

bool SEASON3B::CNewUIMasterLevel::IsSkillAvailable(const _MASTER_SKILLTREE_DATA& data)
{
    const auto skillLevel = CharacterAttribute->MasterSkillInfo[data.Skill].GetSkillLevel();
    const BYTE rank = SkillAttribute[data.Skill].SkillRank;
    return this->CheckParentSkill(data)
        && this->CheckRankPoint(data.Group, rank, skillLevel)
        && this->CheckBeforeSkill(data.Skill, skillLevel)
        && g_csItemOption.IsNonWeaponSkillOrIsSkillEquipped(data.Skill);
}

bool SEASON3B::CNewUIMasterLevel::Render()
{
    EnableAlphaTest();
    this->RecalculateRows();
    UI::AuctionChrome::RenderWindowFrame(static_cast<float>(this->PosX), static_cast<float>(this->PosY),
        static_cast<float>(Tree::WindowWidth), static_cast<float>(Tree::WindowHeight));
    this->RenderColumns();
    this->RenderConnectors();
    this->RenderSkillSlots();
    DisableAlphaBlend();
    this->RenderText();
    this->RenderToolTip();

    return true;
}

bool SEASON3B::CNewUIMasterLevel::Update()
{
    return true;
}

bool SEASON3B::CNewUIMasterLevel::UpdateMouseEvent()
{
    // The close button is part of the frame art, as in the Auction House; only its area needs handling.
    const int closeX = this->PosX + UI::MasterTree::WindowWidth - CloseWidth - CloseMargin;
    if (SEASON3B::CheckMouseIn(closeX, this->PosY + CloseMargin, CloseWidth, CloseHeight) && SEASON3B::IsRelease(VK_LBUTTON))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_MASTER_LEVEL);
        PlayBuffer(SOUND_CLICK01);

        return true;
    }

    this->CheckBtn();

    if (SEASON3B::CheckMouseIn(this->PosX, this->PosY, this->width, this->height) == true)
    {
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIMasterLevel::UpdateKeyEvent()
{
    // 'S' opens and closes the skill tree (see CNewUIHotKey); with the inventory open it is the "my shop" key.
    const bool closeKeyPressed =
        SEASON3B::IsPress('S') == true && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY) == false;
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_MASTER_LEVEL) == false ||
        SEASON3B::IsPress(VK_ESCAPE) == false && closeKeyPressed == false)
    {
        return true;
    }

    g_pNewUISystem->Hide(SEASON3B::INTERFACE_MASTER_LEVEL);

    PlayBuffer(SOUND_CLICK01);

    return false;
}

float SEASON3B::CNewUIMasterLevel::GetLayerDepth()
{
    return 10.1000004;
}

void SEASON3B::CNewUIMasterLevel::LoadImages()
{
    // Only the skill icon sheets are the tree's own. The frame, slots, tabs and close button are the Auction House
    // chrome (loaded by that window) and the tooltip ornaments belong to the buff window.
    LoadBitmap(L"Interface\\new_Master_Icon.jpg", IMAGE_MASTER_INTERFACE, GL_LINEAR, GL_CLAMP, true, false);
    LoadBitmap(L"Interface\\new_Master_Non_Icon.jpg", IMAGE_MASTER_INTERFACE + 1, GL_LINEAR);
}

void SEASON3B::CNewUIMasterLevel::UnloadImages()
{
    for (int i = 0; i < 2; i++)
    {
        DeleteBitmap(i + IMAGE_MASTER_INTERFACE, false);
    }
}

void SEASON3B::CNewUIMasterLevel::BuildInfoChips(InfoChip (&chips)[3]) const
{
    g_pRenderText->SetFont(g_hFont);

    mu_swprintf(chips[0].text, I18N::Game::MasterLevelD, Master_Level_Data.nMLevel);
    mu_swprintf(chips[1].text, I18N::Game::LevelPointD, Master_Level_Data.nMLevelUpMPoint);
    chips[1].highlighted = Master_Level_Data.nMLevelUpMPoint > 0;

    if (Master_Level_Data.lNext_MasterLevel_Experince != 0)
    {
        // The experience of a level is counted from the total of all levels (400 normal levels and the master levels
        // on top), so the percentage is taken over this level's own stretch of experience.
        const __int64 iTotalLevel = Master_Level_Data.nMLevel + 400;
        const __int64 iTOverLevel = iTotalLevel - 255;
        const __int64 iData_Master =
            (((__int64)9 + iTotalLevel) * iTotalLevel * iTotalLevel * (__int64)10)
            + (((__int64)9 + iTOverLevel) * iTOverLevel * iTOverLevel * (__int64)1000);
        const __int64 iBaseExperience = (iData_Master - (__int64)3892250000) / (__int64)2;
        const double fNeedExp = (double)Master_Level_Data.lNext_MasterLevel_Experince - (double)iBaseExperience;
        const double fExp = (double)Master_Level_Data.lMasterLevel_Experince - (double)iBaseExperience;
        mu_swprintf(chips[2].text, I18N::Game::EXP62f, fExp / fNeedExp * 100.0);
    }
    else
    {
        mu_swprintf(chips[2].text, L"%ls", L"EXP: -");
    }

    int total = InfoGap * 2;
    for (auto& chip : chips)
    {
        chip.width = std::max<int>(InfoMinimumWidth, g_pRenderText->MeasureText(chip.text, lstrlen(chip.text)).cx + InfoPadding * 2);
        total += chip.width;
    }
    int x = this->PosX + (Tree::WindowWidth - total) / 2;
    for (auto& chip : chips)
    {
        chip.x = x;
        x += chip.width + InfoGap;
    }
}

void SEASON3B::CNewUIMasterLevel::RenderColumns()
{
    InfoChip chips[3];
    this->BuildInfoChips(chips);
    const auto& field = UI::AuctionChrome::AuctionFieldSkin();
    for (const auto& chip : chips)
    {
        UI::NineSlice::Render(field.texture, static_cast<float>(chip.x), static_cast<float>(this->PosY + InfoY),
            static_cast<float>(chip.width), static_cast<float>(InfoHeight), field.tints.normal);
    }

    for (int column = 0; column < Tree::ColumnCount; ++column)
    {
        const float x = static_cast<float>(this->PosX + Tree::ColumnX(column));
        UI::NineSlice::Render(UI::AuctionChrome::AuctionRowSkin().texture, x, static_cast<float>(this->PosY + Tree::ColumnTop),
            static_cast<float>(Tree::ColumnWidth), static_cast<float>(Tree::ColumnBottom - Tree::ColumnTop), ColumnTint[column]);
        UI::NineSlice::Render(UI::AuctionChrome::AuctionButtonSkin().texture, x + Tree::HeaderInset,
            static_cast<float>(this->PosY + Tree::HeaderTop), static_cast<float>(Tree::ColumnWidth - 2 * Tree::HeaderInset),
            static_cast<float>(Tree::HeaderHeight), this->CategoryPoint[column] > 0 ? HeaderTintActive[column] : HeaderTintIdle[column]);
    }
}

void SEASON3B::CNewUIMasterLevel::RenderConnectors()
{
    for (const auto& entry : this->map_masterData)
    {
        const auto& data = entry.second;
        const bool lit = CharacterAttribute->MasterSkillInfo[data.Skill].GetSkillLevel() > 0 || this->IsSkillAvailable(data);
        RenderConnector(data.ArrowDirection, this->SlotOf(data), this->m_rows, lit ? ConnectorLit : ConnectorDim);
    }
    EndRenderColor();
}

void SEASON3B::CNewUIMasterLevel::RenderSkillSlots()
{
    const auto& field = UI::AuctionChrome::AuctionFieldSkin();
    const bool pressed = SEASON3B::IsPress(VK_LBUTTON);
    for (const auto& entry : this->map_masterData)
    {
        const auto& data = entry.second;
        const auto skillAttribute = &SkillAttribute[data.Skill];
        const auto skillLevel = CharacterAttribute->MasterSkillInfo[data.Skill].GetSkillLevel();
        const bool available = this->IsSkillAvailable(data);
        const auto slot = this->SlotOf(data);
        const bool hovered = SEASON3B::CheckMouseIn(slot.x, slot.y, slot.width, slot.height);

        DWORD tint = skillLevel > 0 ? SlotTintLearned : (available ? SlotTintAvailable : SlotTintLocked);
        if (hovered && !pressed && skillLevel == 0)
        {
            tint = SlotTintHover;
        }
        UI::NineSlice::Render(field.texture, slot.x, slot.y, slot.width, slot.height, tint);

        const float iconWidth = Tree::IconWidth * this->m_rows.scale;
        const float iconHeight = Tree::IconHeight * this->m_rows.scale;
        const int atlas = available ? IMAGE_MASTER_INTERFACE : IMAGE_MASTER_INTERFACE + 1;
        RenderImage(atlas, slot.centreX() - iconWidth / 2.f, slot.centreY() - iconHeight / 2.f, iconWidth, iconHeight,
            (20.f / 512.f) * (skillAttribute->Magic_Icon % 25), (28.f / 512.f) * (skillAttribute->Magic_Icon / 25), 20.f / 512.f, 28.f / 512.f);

        const float badgeX = slot.right() - BadgeWidth + BadgeOverlap;
        const float badgeY = slot.bottom() - BadgeHeight + BadgeOverlap / 2.f;
        UI::NineSlice::Render(field.texture, badgeX, badgeY, BadgeWidth, BadgeHeight, BadgeTint);
        const std::wstring digits = std::to_wstring(skillLevel);
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        if (skillLevel >= data.MaxLevel)
        {
            g_pRenderText->SetTextColor(255, 214, 118, 255);
        }
        else if (skillLevel > 0)
        {
            g_pRenderText->SetTextColor(255, 255, 255, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(160, 160, 160, 255);
        }
        const SIZE size = g_pRenderText->MeasureText(digits.c_str(), static_cast<int>(digits.size()));
        g_pRenderText->RenderText(badgeX, badgeY + (BadgeHeight - size.cy) / 2.f, digits.c_str(), BadgeWidth, 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CNewUIMasterLevel::RenderText() const
{
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->RenderText(static_cast<float>(this->PosX + 15), static_cast<float>(this->PosY + TitleY),
        I18N::Game::Lookup(this->ClassNameTextIndex), static_cast<float>(Tree::WindowWidth - 30), 0, RT3_SORT_CENTER);

    InfoChip chips[3];
    this->BuildInfoChips(chips);
    g_pRenderText->SetFont(g_hFont);
    for (const auto& chip : chips)
    {
        const SIZE size = g_pRenderText->MeasureText(chip.text, lstrlen(chip.text));
        if (chip.highlighted)
        {
            g_pRenderText->SetTextColor(255, 214, 118, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(235, 235, 235, 255);
        }
        g_pRenderText->RenderText(static_cast<float>(chip.x), static_cast<float>(this->PosY + InfoY) + (InfoHeight - size.cy) / 2.f,
            chip.text, static_cast<float>(chip.width), 0, RT3_SORT_CENTER);
    }

    g_pRenderText->SetFont(g_hFontBold);
    for (int column = 0; column < Tree::ColumnCount; ++column)
    {
        wchar_t label[128] = {};
        mu_swprintf(label, I18N::Game::Lookup(this->CategoryTextIndex + column), this->CategoryPoint[column]);
        const SIZE size = g_pRenderText->MeasureText(label, lstrlen(label));
        const DWORD textColor = this->CategoryPoint[column] > 0 ? HeaderTextActive[column] : HeaderTextIdle[column];
        g_pRenderText->SetTextColor(GetRValue(textColor), GetGValue(textColor), GetBValue(textColor), 255);
        g_pRenderText->RenderText(static_cast<float>(this->PosX + Tree::ColumnX(column) + Tree::HeaderInset),
            static_cast<float>(this->PosY + Tree::HeaderTop) + (Tree::HeaderHeight - size.cy) / 2.f, label,
            static_cast<float>(Tree::ColumnWidth - 2 * Tree::HeaderInset), 0, RT3_SORT_CENTER);
    }
    g_pRenderText->SetFont(g_hFont);

    // Hovering the experience readout shows the exact numbers.
    const InfoChip& experience = chips[2];
    if (SEASON3B::IsPress(VK_LBUTTON) == false && SEASON3B::CheckMouseIn(experience.x, this->PosY + InfoY, experience.width, InfoHeight) == true)
    {
        TextList[0][0] = 0;
        TextBold[0] = 0;
        TextListColor[0] = TEXT_COLOR_WHITE;
        mu_swprintf(TextList[0], L"%I64d / %I64d", Master_Level_Data.lMasterLevel_Experince, Master_Level_Data.lNext_MasterLevel_Experince);
        UI::Buffs::Tooltip::Render(experience.x + experience.width / 2, this->PosY + InfoY + InfoHeight + TooltipGap,
            ExpTooltipLines, SEASON3B::CNewUIBuffWindow::IMAGE_BUFF_TOOLTIP_ORNAMENTS);
    }
}

void SEASON3B::CNewUIMasterLevel::RenderToolTip()
{
    for (auto it = this->map_masterData.begin(); it != this->map_masterData.end(); it++)
    {
        const BYTE group = it->second.Group;

        auto Skill = it->second.Skill;

        SKILL_ATTRIBUTE* p = &SkillAttribute[Skill];

        if (p == nullptr)
        {
            break;
        }

        const auto slot = this->SlotOf(it->second);

        if (SEASON3B::IsPress(VK_LBUTTON) == true || SEASON3B::CheckMouseIn(slot.x, slot.y, slot.width, slot.height) == false)
        {
            continue;
        }

        auto mtit = this->map_masterSkillToolTip.find(Skill);

        if (mtit == this->map_masterSkillToolTip.end())
        {
            return;
        }

        auto skillInfo = CharacterAttribute->MasterSkillInfo[Skill];
        const auto skillLevel = skillInfo.GetSkillLevel();
        auto skillValue = skillInfo.GetSkillValue();
        const auto skillNextValue = skillInfo.GetSkillNextValue();

        for (int i = 0; i < 30; i++)
        {
            TextList[i][0] = 0;
        }

        memset(TextBold, 0, sizeof(TextBold));

        for (int i = 0; i < 30; i++)
        {
            TextListColor[i] = i == 0 ? TEXT_COLOR_YELLOW : TEXT_COLOR_WHITE;
        }

        int lineCount = 0;

        mu_swprintf(TextList[lineCount], L"%ls", p->Name);

        TextBold[lineCount] = true;

        lineCount++;

        mu_swprintf(TextList[lineCount], mtit->second.Info1, p->SkillRank, skillLevel, it->second.MaxLevel);

        lineCount++;

        wchar_t buffer[512] = {};

        if (it->second.DefValue == -1.0f)
        {
            mu_swprintf(buffer, mtit->second.Info2);
        }
        else
        {
            mu_swprintf(buffer, mtit->second.Info2, skillLevel != 0 ? skillValue : it->second.DefValue);
        }

        lineCount = this->SetDivideString(buffer, 0, lineCount, 0, 0, true);

        if (skillLevel != 0 && skillLevel < it->second.MaxLevel)
        {
            mu_swprintf(buffer, I18N::Game::NextLevel);

            lineCount = this->SetDivideString(buffer, 0, lineCount, 4, 0, true);

            TextBold[lineCount] = 1;

            mu_swprintf(buffer, mtit->second.Info2, skillNextValue);

            lineCount = this->SetDivideString(buffer, 0, lineCount, 0, 0, true);
        }

        if (skillLevel < it->second.MaxLevel)
        {
            mu_swprintf(buffer, I18N::Game::Requirements3329);

            lineCount = this->SetDivideString(buffer, 0, lineCount, 1, 0, true);

            TextBold[lineCount] = 1;

            mu_swprintf(buffer, mtit->second.Info3, it->second.RequiredPoints);

            if (it->second.RequiredPoints <= Master_Level_Data.nMLevelUpMPoint)
            {
                lineCount = this->SetDivideString(buffer, 0, lineCount, 0, 0, true);
            }
            else
            {
                lineCount = this->SetDivideString(buffer, 0, lineCount, 2, 0, true);
            }
        }

        int iTextColor = this->CheckBeforeSkill(Skill, skillLevel) == true ? 0 : 2;

        mu_swprintf(buffer, mtit->second.Info4);

        lineCount = this->SetDivideString(buffer, 0, lineCount, iTextColor, 0, true);

        if (skillLevel < it->second.MaxLevel && p->SkillRank != 1)
        {
            iTextColor = this->CheckRankPoint(group, p->SkillRank, skillLevel) == true ? 0 : 2;

            mu_swprintf(buffer, mtit->second.Info5);

            lineCount = this->SetDivideString(buffer, 0, lineCount, iTextColor, 0, true);

            for (int i = 0; i < MAX_MASTER_SKILL_REQUIRES; i++)
            {
                const auto RequireSkill = it->second.RequireSkill[i];

                if (RequireSkill >= AT_SKILL_MASTER_BEGIN && RequireSkill <= AT_SKILL_MASTER_END)
                {
                    auto requiredSkill = CharacterAttribute->MasterSkillInfo[RequireSkill];
                    iTextColor = requiredSkill.GetSkillValue() < 10 ? 2 : 0;
                    mu_swprintf(buffer, i == 0 ? mtit->second.Info6 : mtit->second.Info7);
                    lineCount = this->SetDivideString(buffer, 0, lineCount, iTextColor, 0, true);
                }
            }
        }

        // A rule between the skill's name and its description.
        if (lineCount > 1 && lineCount < TooltipLineCapacity)
        {
            for (int line = lineCount; line > 1; --line)
            {
                wcscpy_s(TextList[line], TextList[line - 1]);
                TextListColor[line] = TextListColor[line - 1];
                TextBold[line] = TextBold[line - 1];
            }
            wcscpy_s(TextList[1], L"\n");
            TextListColor[1] = TEXT_COLOR_WHITE;
            TextBold[1] = 0;
            ++lineCount;
        }

        const int ornaments = SEASON3B::CNewUIBuffWindow::IMAGE_BUFF_TOOLTIP_ORNAMENTS;
        if (slot.y > TooltipFlipY)
        {
            UI::Buffs::Tooltip::Render(static_cast<int>(slot.centreX()), static_cast<int>(slot.y) - TooltipGap, lineCount, ornaments, true);
        }
        else
        {
            UI::Buffs::Tooltip::Render(static_cast<int>(slot.centreX()), static_cast<int>(slot.bottom()) + TooltipGap, lineCount, ornaments);
        }

    }
}

bool SEASON3B::CNewUIMasterLevel::CheckBtn()
{
    for (const auto& entry : this->map_masterData)
    {
        this->CheckAttributeArea(entry.second);
    }

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckAttributeArea(const _MASTER_SKILLTREE_DATA& skillData)
{
    if (skillData.Group < 0 || skillData.Group >= 3)
    {
        return false;
    }

    const auto lpskill = &SkillAttribute[skillData.Skill];

    if (lpskill == nullptr)
    {
        return false;
    }

    const auto slot = this->SlotOf(skillData);

    if (!SEASON3B::IsPress(VK_LBUTTON) || SEASON3B::CheckMouseIn(slot.x, slot.y, slot.width, slot.height) == false)
    {
        return true;
    }

    const auto skillPoint = CharacterAttribute->MasterSkillInfo[skillData.Skill].GetSkillLevel();

    PlayBuffer(SOUND_CLICK01);

    if (!this->CheckSkillPoint(Master_Level_Data.nMLevelUpMPoint, skillData, skillPoint))
    {
        return true;
    }

    if (!g_csItemOption.IsNonWeaponSkillOrIsSkillEquipped(skillData.Skill))
    {
        SEASON3B::CreateOkMessageBox(I18N::Game::YouNeedToWearTheRequiredEquipmentToLevelUpThisSkill);
        return true;
    }

    if (!this->CheckParentSkill(skillData)
        || !this->CheckRankPoint(skillData.Group, lpskill->SkillRank, skillPoint)
        || !this->CheckBeforeSkill(skillData.Skill, skillPoint))
    {
        SEASON3B::CreateOkMessageBox(I18N::Game::YouMustMeetAllSkillRequirements);

        return true;
    }

    this->ConsumePoint = skillData.RequiredPoints;
    
    this->CurSkillID = skillData.Skill;

    SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CMaster_Level_Interface));

    MouseLButton = false;

    MouseLButtonPop = false;

    MouseLButtonPush = false;

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckSkillPoint(WORD mLevelUpPoint, const _MASTER_SKILLTREE_DATA& skillData, BYTE skillLevel)
{

    if (skillLevel >= skillData.MaxLevel)
    {
        SEASON3B::CreateOkMessageBox(I18N::Game::YouCanTRaiseAnyMoreLevels);
        return false;
    }

    if (mLevelUpPoint >= skillData.RequiredPoints)
    {
        return true;
    }

    wchar_t Buffer[358] = {};

    mu_swprintf(Buffer, I18N::Game::YouCanTRaiseAnyMoreLevels, skillData.RequiredPoints - mLevelUpPoint);

    SEASON3B::CreateOkMessageBox(Buffer);

    return false;
}

bool SEASON3B::CNewUIMasterLevel::CheckParentSkill(const _MASTER_SKILLTREE_DATA& masterSkill)
{
    for (int i = 0; i < MAX_MASTER_SKILL_REQUIRES; i++)
    {
        const auto requiredSkill = masterSkill.RequireSkill[i];
        if (requiredSkill == AT_SKILL_UNDEFINED)
        {
            return true;
        }

        if (requiredSkill < AT_SKILL_MASTER_BEGIN || requiredSkill > AT_SKILL_MASTER_END)
        {
            return true;
        }

        const auto reqSkillLevel = CharacterAttribute->MasterSkillInfo[requiredSkill].GetSkillLevel();
        if (reqSkillLevel < MASTER_SKILL_LEVEL_REQ_FOR_NEXT_RANK)
        {
            return false;
        }
    }

    return true;
}

bool SEASON3B::CNewUIMasterLevel::CheckRankPoint(BYTE group, BYTE rank, BYTE skillLevel)
{
    if (this->skillPoint[group][rank] < skillLevel)
    {
        this->skillPoint[group][rank] = skillLevel;
    }

    if (rank == 1)
    {
        return true;
    }

    return this->skillPoint[group][rank - 1] >= 10;
}

bool SEASON3B::CNewUIMasterLevel::CheckBeforeSkill(ActionSkillType skill, BYTE skillLevel)
{
    if (skillLevel != 0)
    {
        return true;
    }

    const auto Index = SkillAttribute[skill].SkillBrand;

    if (Index == 0)
    {
        return true;
    }

    const SKILL_ATTRIBUTE* lpSkill = &SkillAttribute[Index];

    if (lpSkill == nullptr)
    {
        return false;
    }

    if (lpSkill->SkillUseType == 4)
    {
        return true;
    }

    for (int i = 0; i < MAX_MAGIC; i++)
    {
        if (CharacterAttribute->Skill[i] == Index)
        {
            return true;
        }
    }

    return false;
}

void SEASON3B::CNewUIMasterLevel::SkillUpgrade(int index, BYTE skillLevel, float value, float nextValue)
{
    const auto it = this->map_masterData.find(index);
    if (it == this->map_masterData.end())
    {
        return;
    }

    const auto realSkill = it->second.Skill;
    const int oldLevel = CharacterAttribute->MasterSkillInfo[realSkill].GetSkillLevel();

    const CSkillTreeInfo skillTreeInfo = { skillLevel, value, nextValue };
    CharacterAttribute->MasterSkillInfo[realSkill] = skillTreeInfo;

    // And update the category points
    const int addedPoints = skillLevel - oldLevel;
    this->CategoryPoint[it->second.Group] += addedPoints;
}

void SEASON3B::CNewUIMasterLevel::ClearSkillTreeData()
{
    if (!map_masterSkillToolTip.empty())
        this->map_masterData.clear();
}

void SEASON3B::CNewUIMasterLevel::ClearSkillTooltipData()
{
    if (!map_masterSkillToolTip.empty())
        this->map_masterSkillToolTip.clear();
}
