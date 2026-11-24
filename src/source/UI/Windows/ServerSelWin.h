//*****************************************************************************
// File: ServerSelWin.h
//*****************************************************************************
#pragma once

#include <string>

#include "UI/Widgets/Win.h"
#include "UI/Widgets/Button.h"
#include "UI/Widgets/GaugeBar.h"
#include "UI/Widgets/WinEx.h"
#include "Network/Server/ServerDisplayNames.h"

#define SSW_SERVER_G_MAX 21
#define SSW_SERVER_MAX 16
#define SSW_DESC_LINE_MAX 2
#define SSW_DESC_ROW_MAX 83
#define SSW_LEFT_SERVER_G_MAX 10
#define SSW_RIGHT_SERVER_G_MAX 10

class CServerGroup;
class CServerInfo;

class CServerSelWin : public CWin
{
private:
    enum SERVER_SELECT_WIN
    {
        SERVER_GROUP_BTN_WIDTH = 108,
        SERVER_GROUP_BTN_HEIGHT = 26,
        SERVER_BTN_WIDTH = 200,
        SERVER_BTN_HEIGHT = 26,
        SERVER_PLATE_HEIGHT = 42,
    };

protected:
    CButton m_aServerGroupBtn[SSW_SERVER_G_MAX];
    CButton m_aServerBtn[SSW_SERVER_MAX];
    CGaugeBar m_aServerGauge[SSW_SERVER_MAX];
    // What each server entry draws: its name, its plate (see ServerNames::ServerPlateStyle) and its load in percent.
    std::wstring m_aServerLabel[SSW_SERVER_MAX];
    Network::ServerNames::ServerPlate m_aServerPlate[SSW_SERVER_MAX];
    bool m_aServerHasPlate[SSW_SERVER_MAX];
    int m_aServerLoad[SSW_SERVER_MAX];
    CSprite m_aBtnDeco[2];
    CSprite m_aArrowDeco[2];
    CWinEx m_winDescription;

    int m_icntServerGroup;
    int m_icntLeftServerGroup;
    int m_icntRightServerGroup;
    int m_icntServer;
    bool m_bTestServerBtn;

    int m_iSelectServerBtnIndex;
    CServerGroup* m_pSelectServerGroup;

    wchar_t m_szDescription[SSW_DESC_LINE_MAX][SSW_DESC_ROW_MAX];

public:
    CServerSelWin();
    virtual ~CServerSelWin();
    void Create();
    void SetPosition(int nXCoord, int nYCoord);
    void UpdateDisplay();
    void Show(bool bShow);
    bool CursorInWin(int nArea);

    // Selects a server group and one of its servers without a click, doing
    // exactly what the two button branches do. `groupName` empty or null picks
    // the first group in the list. Returns false when the list holds no such
    // group or server, or when the server is full.
    bool SelectServer(const wchar_t* groupName, int serverIndex);

protected:
    // Asks the connect server for the game server behind this entry and
    // remembers the choice. Shared by the click path and SelectServer().
    bool ConnectToServer(CServerInfo* pServerInfo);

    // The plate, its pulsing runes, rising sparks and load bar of every server entry.
    void RenderServerPlates();

    void PreRelease();
    void SetServerBtnPosition();
    void SetArrowSpritePosition();
    void ShowServerGBtns();
    void ShowDecoSprite();
    void ShowArrowSprite();
    void ShowServerBtns();
    void UpdateWhileActive(double dDeltaTick);
    void RenderControls();
};
