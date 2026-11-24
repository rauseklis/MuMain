//*****************************************************************************
// Desc: implementation of the CServerSelWin class.
//*****************************************************************************

#include "stdafx.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include "ServerSelWin.h"
#include "Core/Input/Input.h"
#include "UI/Legacy/UIMng.h"
#include "App/Platform/Windows/Local.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "I18N/All.h"

#include "UI/Legacy/UIControls.h"

#include "UI/NewUI/NewUISystem.h"
#include "Network/Server/ServerListManager.h"

#define	SSW_GAP_WIDTH	28
#define	SSW_GAP_HEIGHT	5
#define	SSW_GB_POS_X	16
#define	SSW_GB_POS_Y	19




using namespace SEASON3A;

CServerSelWin::CServerSelWin()
{
    std::fill(std::begin(m_aServerHasPlate), std::end(m_aServerHasPlate), false);
}

CServerSelWin::~CServerSelWin()
{
}

void CServerSelWin::Create()
{
    CWin::Create(0, 0, -2);

    m_iSelectServerBtnIndex = -1;

    int i;

    for (i = 0; i < SSW_SERVER_G_MAX; ++i)
    {
        m_aServerGroupBtn[i].Create(SERVER_GROUP_BTN_WIDTH, SERVER_GROUP_BTN_HEIGHT, BITMAP_LOG_IN, 4, 2, 1, -1, 3);
        CWin::RegisterButton(&m_aServerGroupBtn[i]);
    }

    for (i = 0; i < SSW_SERVER_MAX; ++i)
    {
        // The entry is drawn by RenderServerPlates; the button only takes the hover and the clicks.
        m_aServerBtn[i].Create(SERVER_BTN_WIDTH, SERVER_PLATE_HEIGHT, BITMAP_LOG_IN + 1, 1);
        m_aServerBtn[i].SetArtVisible(false);
        CWin::RegisterButton(&m_aServerBtn[i]);
        m_aServerGauge[i].Create(160, 4, BITMAP_LOG_IN + 2);
    }

    SImgInfo aiiDeco[2] =
    {
        { BITMAP_LOG_IN + 3, 0, 0, 68, 95 },
        { BITMAP_LOG_IN + 3, 68, 0, 68, 95 }
    };
    m_aBtnDeco[0].Create(&aiiDeco[0], 8, 19);
    m_aBtnDeco[1].Create(&aiiDeco[1], 60, 19);

    SImgInfo aiiArrow[2] =
    {
        { BITMAP_LOG_IN + 3, 136, 0, 23, 29 },
        { BITMAP_LOG_IN + 3, 136, 30, 23, 29 }
    };
    m_aArrowDeco[0].Create(&aiiArrow[0], 1, 2);
    m_aArrowDeco[1].Create(&aiiArrow[1], 23, 2);

    SImgInfo aiiDescBg[WE_BG_MAX] =
    {
        { BITMAP_LOG_IN + 11, 0, 0, 4, 4 },
        { BITMAP_LOG_IN + 12, 0, 0, 512, 6 },
        { BITMAP_LOG_IN + 12, 0, 6, 512, 6 },
        { BITMAP_LOG_IN + 13, 0, 0, 3, 4 },
        { BITMAP_LOG_IN + 13, 3, 0, 3, 4 }
    };
    m_winDescription.Create(aiiDescBg, 1, 10);
    m_winDescription.SetLine(10);

    CWin::SetSize((SERVER_GROUP_BTN_WIDTH + SSW_GAP_WIDTH) * 2 + SERVER_BTN_WIDTH, SERVER_BTN_HEIGHT * SSW_SERVER_MAX + SSW_GAP_HEIGHT * 2 + SERVER_GROUP_BTN_HEIGHT + m_winDescription.GetHeight());
}

void CServerSelWin::PreRelease()
{
    int i;

    for (i = 0; i < SSW_SERVER_MAX; ++i)
    {
        m_aServerGauge[i].Release();
    }

    for (i = 0; i < 2; ++i)
    {
        m_aBtnDeco[i].Release();
        m_aArrowDeco[i].Release();
    }

    m_winDescription.Release();
}

void CServerSelWin::SetPosition(int nXCoord, int nYCoord)
{
    CWin::SetPosition(nXCoord, nYCoord);

    int nServerGBtnWidth = m_aServerGroupBtn[0].GetWidth();
    int nServerGBtnHeight = m_aServerGroupBtn[0].GetHeight();
    int nServerBtnWidth = m_aServerBtn[0].GetWidth();
    int nServerBtnHeight = m_aServerBtn[0].GetHeight();
    int nDescGgHeight = m_winDescription.GetHeight();
    int nBtnPosY;
    int i;

    int nServerGBtnBasePosY = nYCoord + CWin::GetHeight() - (nServerGBtnHeight * 11 + SSW_GAP_HEIGHT * 2 + nDescGgHeight);
    int nRServerGBtnPosX = nXCoord + nServerGBtnWidth + nServerBtnWidth + (SSW_GAP_WIDTH * 2);

    int icntServreGroup = 0;
    m_aServerGroupBtn[icntServreGroup++].SetPosition(nXCoord + (CWin::GetWidth() - nServerGBtnWidth) / 2, nYCoord + CWin::GetHeight() - nServerGBtnHeight - SSW_GAP_HEIGHT - nDescGgHeight);

    for (i = 0; i < SSW_LEFT_SERVER_G_MAX; i++)
    {
        nBtnPosY = nServerGBtnBasePosY + nServerGBtnHeight * i;
        m_aServerGroupBtn[icntServreGroup++].SetPosition(nXCoord, nBtnPosY);
    }

    for (i = 0; i < SSW_RIGHT_SERVER_G_MAX; i++)
    {
        nBtnPosY = nServerGBtnBasePosY + nServerGBtnHeight * i;
        m_aServerGroupBtn[icntServreGroup++].SetPosition(nRServerGBtnPosX, nBtnPosY);
    }

    m_winDescription.SetPosition(nXCoord - ((m_winDescription.GetWidth() - CWin::GetWidth()) / 2), nYCoord + CWin::GetHeight() - m_winDescription.GetHeight());

    m_aBtnDeco[0].SetPosition(m_aServerGroupBtn[1].GetXPos(), m_aServerGroupBtn[1].GetYPos());
    m_aBtnDeco[1].SetPosition(m_aServerGroupBtn[SSW_LEFT_SERVER_G_MAX + 1].GetXPos() + SERVER_GROUP_BTN_WIDTH, m_aServerGroupBtn[SSW_LEFT_SERVER_G_MAX + 1].GetYPos());

    int a = m_aServerGroupBtn[1].GetXPos();
}

void CServerSelWin::SetServerBtnPosition()
{
    if (m_iSelectServerBtnIndex == -1)
        return;

    int nServerBtnPosX = m_aServerGroupBtn[1].GetXPos() + m_aServerGroupBtn[0].GetWidth() + SSW_GAP_WIDTH;

    int nServerBtnHeight = m_aServerBtn[0].GetHeight();

    int nLServerGBtnHeightSum = m_aServerGroupBtn[1].GetHeight() * 10;

    int nServerBtnHeightSum = nServerBtnHeight * m_icntServer;

    int nLServerGBtnTop = m_aServerGroupBtn[1].GetYPos();

    int nServerBtnBasePosY = nLServerGBtnHeightSum > nServerBtnHeightSum ? nLServerGBtnTop : nLServerGBtnTop - (nServerBtnHeightSum - nLServerGBtnHeightSum);

    for (int i = 0; i < m_pSelectServerGroup->GetServerSize(); i++)
    {
        m_aServerBtn[i].SetPosition(nServerBtnPosX, nServerBtnBasePosY + nServerBtnHeight * i);
        m_aServerGauge[i].SetPosition(m_aServerBtn[i].GetXPos() + SSW_GB_POS_X, m_aServerBtn[i].GetYPos() + SSW_GB_POS_Y);
    }
}

void CServerSelWin::SetArrowSpritePosition()
{
    if (m_iSelectServerBtnIndex == -1)
        return;

    if ((m_iSelectServerBtnIndex >= 0) && (m_iSelectServerBtnIndex <= SSW_LEFT_SERVER_G_MAX))
    {
        m_aArrowDeco[0].SetPosition(m_aServerGroupBtn[m_iSelectServerBtnIndex].GetXPos() + SERVER_GROUP_BTN_WIDTH, m_aServerGroupBtn[m_iSelectServerBtnIndex].GetYPos());
    }
    else if ((m_iSelectServerBtnIndex > SSW_LEFT_SERVER_G_MAX) && (m_iSelectServerBtnIndex < SSW_SERVER_G_MAX))
    {
        m_aArrowDeco[1].SetPosition(m_aServerGroupBtn[m_iSelectServerBtnIndex].GetXPos(), m_aServerGroupBtn[m_iSelectServerBtnIndex].GetYPos());
    }
}

void CServerSelWin::UpdateDisplay()
{
    m_pSelectServerGroup = NULL;
    m_icntServerGroup = 0;
    m_icntServer = 0;
    m_icntLeftServerGroup = 0;
    m_icntRightServerGroup = 0;
    m_bTestServerBtn = false;

    DWORD adwServerGBtnClr[BTN_IMG_MAX] =
    {
        CLRDW_BR_GRAY, CLRDW_BR_GRAY, CLRDW_WHITE, 0,
        CLRDW_BR_GRAY, CLRDW_BR_GRAY, CLRDW_WHITE, 0
    };

    m_icntServerGroup = g_ServerListManager->GetServerGroupSize();

    if (m_icntServerGroup < 1)
        return;

    CServerGroup* pServerGroup = NULL;

    g_ServerListManager->SetFirst();

    while (g_ServerListManager->GetNext(pServerGroup))
    {
        if (pServerGroup->m_iWidthPos == CServerGroup::SBP_CENTER)
        {
            if (m_bTestServerBtn == true)
                continue;

            m_aServerGroupBtn[0].SetText(pServerGroup->m_szName, adwServerGBtnClr);
            pServerGroup->m_iBtnPos = 0;
            m_bTestServerBtn = true;
        }
        else if (pServerGroup->m_iWidthPos == CServerGroup::SBP_LEFT)
        {
            if (m_icntLeftServerGroup >= SSW_LEFT_SERVER_G_MAX)
                continue;

            m_aServerGroupBtn[m_icntLeftServerGroup + 1].SetText(pServerGroup->m_szName, adwServerGBtnClr);
            pServerGroup->m_iBtnPos = m_icntLeftServerGroup + 1;

            m_icntLeftServerGroup++;
        }
        else if (pServerGroup->m_iWidthPos == CServerGroup::SBP_RIGHT)
        {
            if (m_icntRightServerGroup >= SSW_RIGHT_SERVER_G_MAX)
                continue;

            m_aServerGroupBtn[SSW_LEFT_SERVER_G_MAX + m_icntRightServerGroup + 1].SetText(pServerGroup->m_szName, adwServerGBtnClr);
            pServerGroup->m_iBtnPos = SSW_LEFT_SERVER_G_MAX + m_icntRightServerGroup + 1;

            m_icntRightServerGroup++;
        }
    }

    ShowServerGBtns();
    ShowDecoSprite();

    memset(m_szDescription, 0, sizeof(char) * SSW_DESC_LINE_MAX * SSW_DESC_ROW_MAX);

    if (m_iSelectServerBtnIndex != -1)
    {
        m_pSelectServerGroup = g_ServerListManager->GetServerGroupByBtnPos(m_iSelectServerBtnIndex);
    }

    if (m_pSelectServerGroup == NULL)
        return;

    m_icntServer = m_pSelectServerGroup->GetServerSize();

    if (m_icntServer < 1)
        return;

    CServerInfo* pServerInfo = NULL;

    m_pSelectServerGroup->SetFirst();

    int icntServer = 0;
    while (m_pSelectServerGroup->GetNext(pServerInfo))
    {
        m_aServerHasPlate[icntServer] =
            Network::ServerNames::ServerPlateStyle(pServerInfo->m_iIndex, m_aServerPlate[icntServer]);
        m_aServerLabel[icntServer] = pServerInfo->m_bName;
        m_aServerLoad[icntServer] = pServerInfo->m_iPercent;
        icntServer++;
    }

    ::SeparateTextIntoLines(m_pSelectServerGroup->m_szDescription, m_szDescription[0], SSW_DESC_LINE_MAX, SSW_DESC_ROW_MAX);

    SetArrowSpritePosition();
    SetServerBtnPosition();
    ShowArrowSprite();
    ShowServerBtns();
}

void CServerSelWin::Show(bool bShow)
{
    CWin::Show(bShow);
}

void CServerSelWin::ShowServerGBtns()
{
    int i;

    if (m_bTestServerBtn == true)
    {
        m_aServerGroupBtn[0].Show(CWin::m_bShow);
    }
    else
    {
        m_aServerGroupBtn[0].Show(false);
    }

    for (i = 1; i < m_icntLeftServerGroup + 1; i++)
    {
        m_aServerGroupBtn[i].Show(CWin::m_bShow);
    }
    for (; i < SSW_LEFT_SERVER_G_MAX; ++i)
    {
        m_aServerGroupBtn[i].Show(false);
    }

    for (i = SSW_LEFT_SERVER_G_MAX + 1; i < SSW_RIGHT_SERVER_G_MAX + 1 + m_icntRightServerGroup; i++)
    {
        m_aServerGroupBtn[i].Show(CWin::m_bShow);
    }
    for (; i < SSW_SERVER_G_MAX; i++)
    {
        m_aServerGroupBtn[i].Show(false);
    }
}

void CServerSelWin::ShowDecoSprite()
{
    if (m_icntLeftServerGroup > 0)
    {
        m_aBtnDeco[0].Show(CWin::m_bShow);
    }
    else
    {
        m_aBtnDeco[0].Show(false);
    }

    if (m_icntRightServerGroup > 0)
    {
        m_aBtnDeco[1].Show(CWin::m_bShow);
    }
    else
    {
        m_aBtnDeco[1].Show(false);
    }
}

void CServerSelWin::ShowArrowSprite()
{
    if ((m_iSelectServerBtnIndex >= 0) && (m_iSelectServerBtnIndex <= SSW_LEFT_SERVER_G_MAX))
    {
        m_aArrowDeco[0].Show(CWin::m_bShow);
        m_aArrowDeco[1].Show(false);
    }
    else if ((m_iSelectServerBtnIndex > SSW_LEFT_SERVER_G_MAX) && (m_iSelectServerBtnIndex < SSW_SERVER_G_MAX))
    {
        m_aArrowDeco[0].Show(false);
        m_aArrowDeco[1].Show(CWin::m_bShow);
    }
    else
    {
        m_aArrowDeco[0].Show(false);
        m_aArrowDeco[1].Show(false);
    }
}

void CServerSelWin::ShowServerBtns()
{
    if (m_iSelectServerBtnIndex == -1)
    {
        m_winDescription.Show(false);
        return;
    }

    int i;
    for (i = 0; i < m_icntServer; i++)
    {
        m_aServerBtn[i].Show(CWin::m_bShow);
        m_aServerGauge[i].Show(CWin::m_bShow);
    }
    for (; i < SSW_SERVER_MAX; i++)
    {
        m_aServerBtn[i].Show(false);
        m_aServerGauge[i].Show(false);
    }

    m_winDescription.Show(CWin::m_bShow);
}

bool CServerSelWin::CursorInWin(int nArea)
{
    if (!CWin::m_bShow)
        return false;

    switch (nArea)
    {
    case WA_MOVE:
        return false;
    }

    return CWin::CursorInWin(nArea);
}

void CServerSelWin::UpdateWhileActive(double dDeltaTick)
{
    int i;

    for (i = 0; i < SSW_SERVER_G_MAX; i++)
    {
        if (m_aServerGroupBtn[i].IsClick())
        {
            if (m_iSelectServerBtnIndex != -1)
            {
                m_aServerGroupBtn[m_iSelectServerBtnIndex].SetCheck(false);
            }

            m_aServerGroupBtn[i].SetCheck(true);
            m_iSelectServerBtnIndex = i;

            SocketClient->ToConnectServer()->SendServerListRequest();
        }
    }

    if (m_pSelectServerGroup == NULL)
        return;

    CServerInfo* pServerInfo = NULL;
    for (i = 0; i < m_icntServer; i++)
    {
        if (m_aServerBtn[i].IsClick())
        {
            pServerInfo = m_pSelectServerGroup->GetServerInfo(i);

            if (pServerInfo == NULL)
                return;

            if (pServerInfo->m_iPercent < 100)
            {
                ConnectToServer(pServerInfo);
                break;
            }
            else if (pServerInfo->m_iPercent < 128)
            {
                CUIMng::Instance().PopUpMsgWin(MESSAGE_SERVER_BUSY);
            }
        }
    }
}

bool CServerSelWin::ConnectToServer(CServerInfo* pServerInfo)
{
    if (pServerInfo == NULL || m_pSelectServerGroup == NULL)
        return false;

    CUIMng::Instance().HideWin(this);

    SocketClient->ToConnectServer()->SendConnectionInfoRequest(static_cast<uint16_t>(pServerInfo->m_iConnectIndex));
    g_pSystemLogBox->AddText(I18N::Game::ConnectingToTheServer, SEASON3B::TYPE_SYSTEM_MESSAGE);
    g_pSystemLogBox->AddText(I18N::Game::PleaseWait, SEASON3B::TYPE_SYSTEM_MESSAGE);

    g_ServerListManager->SetSelectServerInfo(m_pSelectServerGroup->m_szName, pServerInfo->m_iIndex,
                                             pServerInfo->m_byNonPvP);

    return true;
}

bool CServerSelWin::SelectServer(const wchar_t* groupName, int serverIndex)
{
    CServerGroup* pChosenGroup = NULL;
    CServerGroup* pServerGroup = NULL;

    g_ServerListManager->SetFirst();
    while (g_ServerListManager->GetNext(pServerGroup))
    {
        const bool bWanted = (groupName == NULL || groupName[0] == L'\0')
                                 ? (pChosenGroup == NULL)
                                 : (wcscmp(pServerGroup->m_szName, groupName) == 0);
        if (bWanted)
        {
            pChosenGroup = pServerGroup;
            break;
        }
    }

    if (pChosenGroup == NULL)
        return false;

    // A group the display could not place has no button to check, so there is
    // nothing this entry point can select.
    const int iBtnPos = pChosenGroup->m_iBtnPos;
    if (iBtnPos < 0 || iBtnPos >= SSW_SERVER_G_MAX)
        return false;

    // Same two steps the click path takes: mark the group, rebuild its server
    // buttons, then connect to the chosen server. The group has to be marked
    // before the server can be looked up, so a failure after this point puts
    // the previous selection back rather than leaving the screen on a group
    // the caller never reached.
    const int iPreviousBtnIndex = m_iSelectServerBtnIndex;
    if (m_iSelectServerBtnIndex != -1)
        m_aServerGroupBtn[m_iSelectServerBtnIndex].SetCheck(false);

    m_iSelectServerBtnIndex = iBtnPos;
    m_aServerGroupBtn[m_iSelectServerBtnIndex].SetCheck(true);
    UpdateDisplay();

    // UpdateDisplay re-derives m_pSelectServerGroup from the button index,
    // which is only the same group while the buttons and the list agree;
    // the group this call chose is the one it must connect to.
    if (m_pSelectServerGroup != pChosenGroup)
    {
        m_pSelectServerGroup = pChosenGroup;
    }

    CServerInfo* pServerInfo = m_pSelectServerGroup->GetServerInfo(serverIndex);
    if (pServerInfo == NULL || pServerInfo->m_iPercent >= 100)
    {
        m_aServerGroupBtn[m_iSelectServerBtnIndex].SetCheck(false);
        m_iSelectServerBtnIndex = iPreviousBtnIndex;
        if (m_iSelectServerBtnIndex != -1)
            m_aServerGroupBtn[m_iSelectServerBtnIndex].SetCheck(true);
        UpdateDisplay();
        return false;
    }

    return ConnectToServer(pServerInfo);
}

void CServerSelWin::RenderServerPlates()
{
    // 640x480 layout units, like the rest of the window. One cell of the atlas is the 200 x 40 plate with 4 units of
    // shadow room around it; the plate sits one unit below the top of its 42 units high slot.
    constexpr float CELL_WIDTH = 208.0f;
    constexpr float CELL_HEIGHT = 48.0f;
    constexpr float SHADOW_ROOM = 4.0f;
    constexpr float PLATE_TOP = 1.0f;
    // The engine pads every texture up to a power of two with the picture in the top-left corner, so the 2 x 4 cells
    // (416 x 192) sit in a 512 x 256 texture and a cell is not half or a quarter of it.
    constexpr float COLUMN_STEP = CELL_WIDTH / 512.0f;
    constexpr float ROW_STEP = CELL_HEIGHT / 256.0f;
    constexpr int ROW_HOVER = 1;
    constexpr int ROW_PRESSED = 2;
    constexpr int ROW_RUNES = 3;
    constexpr float PULSE_PERIOD_MS = 2800.0f;
    constexpr float TWO_PI = 6.2831853f;
    constexpr int SPARKS = 4;

    const DWORD tick = GetTickCount();
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(tick % 100000u) * TWO_PI / PULSE_PERIOD_MS);

    for (int i = 0; i < m_icntServer; ++i)
    {
        if (!m_aServerBtn[i].IsShow())
            continue;

        const bool hover = m_aServerBtn[i].CursorInObject() != FALSE;
        const bool pressed = hover && CInput::Instance().IsLBtnDn();
        const float x = static_cast<float>(m_aServerBtn[i].GetXPos()) / g_fScreenRate_x;
        const float y = static_cast<float>(m_aServerBtn[i].GetYPos()) / g_fScreenRate_y;
        const float w = static_cast<float>(m_aServerBtn[i].GetWidth()) / g_fScreenRate_x;

        if (m_aServerHasPlate[i])
        {
            const Network::ServerNames::ServerPlate& plate = m_aServerPlate[i];
            const float u = static_cast<float>(plate.column) * COLUMN_STEP;
            const int row = pressed ? ROW_PRESSED : (hover ? ROW_HOVER : 0);
            const float left = x - SHADOW_ROOM;
            const float top = y + PLATE_TOP - SHADOW_ROOM;

            EnableAlphaTest();
            RenderBitmap(BITMAP_LOG_IN + 15, left, top, CELL_WIDTH, CELL_HEIGHT, u, static_cast<float>(row) * ROW_STEP,
                         COLUMN_STEP, ROW_STEP);

            // The runes and the rim light breathe over the plate; brighter under the cursor, fainter when pressed.
            const float runeAlpha = pressed ? 0.12f : (hover ? 0.45f + 0.4f * pulse : 0.15f + 0.55f * pulse);
            const unsigned int runeColor = (static_cast<unsigned int>(runeAlpha * 255.0f) << 24) | 0x00FFFFFFu;
            RenderColorBitmap(BITMAP_LOG_IN + 15, left, top, CELL_WIDTH, CELL_HEIGHT, u,
                              static_cast<float>(ROW_RUNES) * ROW_STEP, COLUMN_STEP, ROW_STEP, runeColor);

            // A few sparks rising from the lower edge.
            const unsigned int sparkRgb = plate.glow & 0x00FFFFFFu;
            for (int k = 0; k < SPARKS; ++k)
            {
                const float period = 2300.0f + 610.0f * static_cast<float>(k);
                const float phase =
                    std::fmod(static_cast<float>(tick % 200000u) / period + 0.27f * static_cast<float>(k), 1.0f);
                const float sx =
                    x + 52.0f + 34.0f * static_cast<float>(k) + 3.0f * std::sin(phase * TWO_PI + static_cast<float>(k));
                const float sy = y + PLATE_TOP + 34.0f - phase * 26.0f;
                const float fade = std::sin(phase * 3.1415927f);
                RenderColorQuadARGB(sx, sy, 1.3f, 1.3f,
                                    (static_cast<unsigned int>(fade * (hover ? 235.0f : 170.0f)) << 24) | sparkRgb);
            }
            EndRenderColor();
        }
        else
        {
            // A server without plate art: a plain dark plate with its name.
            EnableAlphaTest();
            RenderColorQuadARGB(x, y + PLATE_TOP, w, 40.0f, hover ? 0xD0302C28u : 0xD0181614u);
            EndRenderColor();
            g_pRenderText->SetFont(g_hFixFont);
            g_pRenderText->SetBgColor(0);
            g_pRenderText->SetTextColor(CLRDW_BR_GRAY);
            g_pRenderText->RenderText(static_cast<int>(x), static_cast<int>(y + 15.0f), m_aServerLabel[i].c_str(),
                                      static_cast<int>(w), 0, RT3_SORT_CENTER);
        }

        // The load of the server fills the lower rune line from the left; nothing while the server is empty.
        const int load = std::min(m_aServerLoad[i], 100);
        if (load > 0)
        {
            const float barX = x + 14.0f;
            const float barWidth = w - 28.0f;
            const unsigned int barRgb = load < 50 ? 0x6EDC78u : (load < 85 ? 0xF0BE46u : 0xE6463Cu);
            EnableAlphaTest();
            RenderColorQuadARGB(barX, y + PLATE_TOP + 37.1f, barWidth * static_cast<float>(load) / 100.0f, 1.0f,
                                0xE6000000u | barRgb);
            EndRenderColor();
        }
    }
}

void CServerSelWin::RenderControls()
{
    int i = 0;

    g_pRenderText->SetFont(g_hFixFont);
    g_pRenderText->SetTextColor(CLRDW_WHITE);
    g_pRenderText->SetBgColor(0);

    CWin::RenderButtons();
    RenderServerPlates();

    if (m_pSelectServerGroup != NULL)
    {
        if (m_pSelectServerGroup->m_bPvPServer == true)
        {
            g_pRenderText->SetTextColor(ARGB(255, 255, 255, 255));
            g_pRenderText->RenderText(90, 164 - 60, I18N::Game::SinceHelheimServer);
            g_pRenderText->RenderText(90, 164 - 45, I18N::Game::TendsToBeCrowded);
            g_pRenderText->RenderText(90, 164 - 30, I18N::Game::WeRecommendThatYouUseOtherServers);
        }
    }
}