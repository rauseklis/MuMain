// NewUIAuctionWindow.h: interface for the CNewUIAuctionWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "Guild/NewUIGuildInfoWindow.h"

namespace SEASON3B
{
    // The standalone Auction House window. Four tabs: Browse, Sell, My Listings, Mailbox. This slice wires the
    // frame, the tab chrome, and the open/close lifecycle; the tab bodies are filled in by later work (the
    // design spec's tasks 4.3-4.5) and are empty placeholders here.
    class CNewUIAuctionWindow : public CNewUIObj
    {
    public:
        enum TAB
        {
            TAB_BROWSE = 0,
            TAB_SELL,
            TAB_MY_LISTINGS,
            TAB_MAILBOX,
            TAB_COUNT,
        };

        enum IMAGE_LIST
        {
            IMAGE_AUCTION_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,           // newui_msgbox_back.jpg
            IMAGE_AUCTION_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,       // newui_item_back01.tga
            IMAGE_AUCTION_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,     // newui_item_back02-L.tga
            IMAGE_AUCTION_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,   // newui_item_back02-R.tga
            IMAGE_AUCTION_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM, // newui_item_back03.tga
            IMAGE_AUCTION_CLOSE_BTN = CNewUIMyInventory::IMAGE_INVENTORY_EXIT_BTN, // newui_exit_00.tga
            IMAGE_AUCTION_TAB_BTN = CNewUIGuildInfoWindow::IMAGE_GUILDINFO_TAB_BUTTON, // newui_guild_tab04.tga
        };

        static constexpr int WINDOW_WIDTH = 400;
        static constexpr int WINDOW_HEIGHT = 429;

        CNewUIAuctionWindow();
        ~CNewUIAuctionWindow() override;

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        void SetPos(int x, int y);

        // Called once when the window becomes visible: resets to the Browse tab and (once wired in a later
        // slice) sends the open request.
        void OpeningProcess();
        // Called once when the window is hidden: closes the inventory window if this window is the one that
        // opened it for the Sell tab (not implemented until the Sell tab exists).
        void ClosingProcess();

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override;

    private:
        void LoadImages();
        void UnloadImages();
        void RenderFrame();
        bool BtnProcess();

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;

        CNewUIRadioGroupButton m_TabBtn;
        int m_iCurrentTab;

        CNewUIButton m_BtnClose;
    };
}
