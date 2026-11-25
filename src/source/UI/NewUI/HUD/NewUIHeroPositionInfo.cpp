// NewUIHeroPositionInfo.cpp: implementation of the CNewUIHeroPositionInfo class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/HUD/NewUIHeroPositionInfo.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "GameLogic/ExpHunt/ExpHuntTracker.h"
#include "UI/NewUI/NewUISystem.h"
#include "World/MapInfra/MapManager.h"
#include "MUHelper/MuHelper.h"

using namespace SEASON3B;

namespace
{
namespace Bar = UI::HUD::TopLeftBar;

constexpr int kTextBufferLength = 64;
const DWORD kTextColor = RGBA(255, 255, 255, 255);
const DWORD kWhite = RGBA(255, 255, 255, 255);
constexpr wchar_t kEllipsis[] = L"...";
constexpr size_t kEllipsisLength = 3;

Bar::Rect ScreenRect(const Bar::Rect& local, const POINT& origin)
{
    return Bar::Offset(local, origin.x, origin.y);
}

// Shortens `text` with "..." until it is at most `maxWidth` logical pixels wide in the font that is currently set.
void FitText(wchar_t* text, int maxWidth)
{
    size_t length = wcslen(text);
    while (length > 1 && g_pRenderText->MeasureText(text, static_cast<int>(length)).cx > maxWidth)
    {
        --length;
        if (length > kEllipsisLength)
        {
            wmemcpy(text + length - kEllipsisLength, kEllipsis, kEllipsisLength);
        }
        text[length] = L'\0';
    }
}

// Draws `text` centred inside `box` (logical pixels), both ways, using the font that is currently set.
void RenderCentredText(const wchar_t* text, const Bar::Rect& box, DWORD color)
{
    const SIZE size = g_pRenderText->MeasureText(text, static_cast<int>(wcslen(text)));
    const int top = box.y + (box.height - size.cy) / 2;
    g_pRenderText->SetTextColor(color);
    g_pRenderText->RenderText(box.x, top, text, box.width, 0, RT3_SORT_CENTER);
}
} // namespace

CNewUIHeroPositionInfo::CNewUIHeroPositionInfo()
{
    m_pNewUIMng = NULL;
    m_Pos.x = m_Pos.y = 0;
    m_CurHeroPosition.x = m_CurHeroPosition.y = 0;
}

CNewUIHeroPositionInfo::~CNewUIHeroPositionInfo()
{
    Release();
}

//---------------------------------------------------------------------------------------------
// Create
bool CNewUIHeroPositionInfo::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_HERO_POSITION_INFO, this);

    SetPos(x, y);
    LoadImages();
    PlaceButtons();

    m_BtnConfig.ChangeToolTipText(&I18N::Game::OfficialMUHelperSetting, false);
    m_BtnStart.ChangeToolTipText(&I18N::Game::StartOfficialMUHelper, false);
    m_BtnStop.ChangeToolTipText(&I18N::Game::StopOfficialMUHelper, false);
    m_BtnExpHunt.ChangeToolTipText(&I18N::Game::ExperienceHunter, false);

    Show(true);

    return true;
}

void CNewUIHeroPositionInfo::Release()
{
    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }
}

void CNewUIHeroPositionInfo::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    PlaceButtons();
}

// The start and stop buttons share one spot; only one of them is shown and clicked at a time.
void CNewUIHeroPositionInfo::PlaceButtons()
{
    const auto place = [this](CNewUIButton& button, Bar::Button which)
    {
        const Bar::Rect rect = ScreenRect(Bar::ButtonRect(which), m_Pos);
        button.ChangeButtonInfo(rect.x, rect.y, rect.width, rect.height);
    };
    place(m_BtnConfig, Bar::Button::Settings);
    place(m_BtnStart, Bar::Button::MuHelper);
    place(m_BtnStop, Bar::Button::MuHelper);
    place(m_BtnExpHunt, Bar::Button::ExperienceHunter);
}

bool CNewUIHeroPositionInfo::BtnProcess()
{
    if (m_BtnConfig.UpdateMouseEvent())
    {
        g_pNewUISystem->Toggle(SEASON3B::INTERFACE_MUHELPER);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    CNewUIButton& helperButton = MUHelper::g_MuHelper.IsActive() ? m_BtnStop : m_BtnStart;
    if (helperButton.UpdateMouseEvent())
    {
        MUHelper::g_MuHelper.Toggle();

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (ProcessExpHuntButton())
    {
        return true;
    }

    return false;
}

bool CNewUIHeroPositionInfo::ProcessExpHuntButton()
{
    if (!m_BtnExpHunt.UpdateMouseEvent())
    {
        return false;
    }

    g_pNewUISystem->ToggleExpHunt();
    PlayBuffer(SOUND_CLICK01);
    return true;
}

bool CNewUIHeroPositionInfo::UpdateMouseEvent()
{
    if (true == BtnProcess())
    {
        return false;
    }

    const Bar::Rect body = ScreenRect(Bar::Rect{0, 0, Bar::kLeftWidth + Bar::kMiddleWidth, Bar::kFrameHeight}, m_Pos);
    const Bar::Rect right = ScreenRect(Bar::kRightPiece, m_Pos);
    if (CheckMouseIn(body.x, body.y, body.width, body.height) ||
        CheckMouseIn(right.x, right.y, right.width, right.height))
    {
        return false;
    }

    return true;
}

bool CNewUIHeroPositionInfo::UpdateKeyEvent()
{
    return true;
}

bool CNewUIHeroPositionInfo::Update()
{
    if ((IsVisible() == true) && (Hero != NULL))
    {
        m_CurHeroPosition.x = (Hero->PositionX);
        m_CurHeroPosition.y = (Hero->PositionY);
    }

    return true;
}

// Draws a piece of the atlas so that its edges fall on whole screen pixels and the texture is sampled exactly
// (no half-texel inset), which keeps the art as sharp as it was drawn.
void CNewUIHeroPositionInfo::RenderAtlasRect(const Bar::Rect& destination, const Bar::Rect& source)
{
    const Bar::SnappedRect snapped =
        Bar::SnapRect(destination, g_fScreenRate_x, g_fScreenRate_y, g_fScreenOffset_x, g_fScreenOffset_y);
    RenderImage(IMAGE_HERO_POSITION_INFO_ATLAS, snapped.x, snapped.y, snapped.width, snapped.height,
                Bar::AtlasUv(source.x), Bar::AtlasUv(source.y), Bar::AtlasUv(source.width), Bar::AtlasUv(source.height),
                kWhite);
}

void CNewUIHeroPositionInfo::RenderFrame()
{
    RenderAtlasRect(ScreenRect(Bar::kLeftPiece, m_Pos), Bar::kLeftSource);
    RenderAtlasRect(ScreenRect(Bar::kMiddlePiece, m_Pos), Bar::kMiddleSource);
    RenderAtlasRect(ScreenRect(Bar::kRightPiece, m_Pos), Bar::kRightSource);
}

void CNewUIHeroPositionInfo::RenderButtonFace(CNewUIButton& button, Bar::Button which, Bar::Face face)
{
    const BUTTON_STATE state = button.GetBTState();
    const Bar::ButtonState faceState = state == BUTTON_STATE_DOWN   ? Bar::ButtonState::Down
                                       : state == BUTTON_STATE_OVER ? Bar::ButtonState::Over
                                                                    : Bar::ButtonState::Up;
    RenderAtlasRect(ScreenRect(Bar::ButtonRect(which), m_Pos), Bar::FaceRect(face, faceState));
}

void CNewUIHeroPositionInfo::RenderTexts()
{
    wchar_t text[kTextBufferLength] = {};
    mu_swprintf(text, L"%ls (%d , %d)", gMapManager.GetMapName(gMapManager.WorldActive), m_CurHeroPosition.x,
                m_CurHeroPosition.y);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    const Bar::Rect box = ScreenRect(Bar::kTextBox, m_Pos);
    FitText(text, box.width - 2 * Bar::kTextInset);
    RenderCentredText(text, box, kTextColor);
}

bool CNewUIHeroPositionInfo::Render()
{
    EnableAlphaTest();

    RenderFrame();

    const bool helperRunning = MUHelper::g_MuHelper.IsActive();
    const bool experienceHunting = GameLogic::ExpHunt::IsActive();
    RenderButtonFace(m_BtnConfig, Bar::Button::Settings, Bar::Face::Settings);
    RenderButtonFace(helperRunning ? m_BtnStop : m_BtnStart, Bar::Button::MuHelper,
                     helperRunning ? Bar::Face::MuHelperStop : Bar::Face::MuHelperStart);
    RenderButtonFace(m_BtnExpHunt, Bar::Button::ExperienceHunter,
                     experienceHunting ? Bar::Face::ExperienceOn : Bar::Face::ExperienceOff);

    RenderTexts();

    // The buttons draw only their tooltip (they have no image of their own).
    m_BtnConfig.Render();
    (helperRunning ? m_BtnStop : m_BtnStart).Render();
    m_BtnExpHunt.Render();

    DisableAlphaBlend();
    return true;
}

float CNewUIHeroPositionInfo::GetLayerDepth()
{
    return 4.3f;
}

void CNewUIHeroPositionInfo::OpenningProcess()
{
}

void CNewUIHeroPositionInfo::ClosingProcess()
{
}

void CNewUIHeroPositionInfo::SetCurHeroPosition(int x, int y)
{
    m_CurHeroPosition.x = x;
    m_CurHeroPosition.y = y;
}

void CNewUIHeroPositionInfo::LoadImages()
{
    LoadBitmap(L"Interface\\TopLeftBar\\TopLeftBar.tga", IMAGE_HERO_POSITION_INFO_ATLAS, GL_LINEAR);
}

void CNewUIHeroPositionInfo::UnloadImages()
{
    DeleteBitmap(IMAGE_HERO_POSITION_INFO_ATLAS);
}
