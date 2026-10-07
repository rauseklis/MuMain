// NewUIAuctionWindow.h: interface for the CNewUIAuctionWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Widgets/NewUIComboBox.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "Guild/NewUIGuildInfoWindow.h"
#include "UI/NewUI/AuctionHouse/AuctionWireResponses.h"

namespace SEASON3B
{
    // The standalone Auction House window. Four tabs: Browse, Sell, My Listings, Mailbox. This slice wires the
    // frame, the tab chrome, and the open/close lifecycle; the tab bodies are filled in by later work (the
    // design spec's tasks 4.3-4.5) and are empty placeholders here.
    class CNewUIAuctionWindow : public CNewUIObj, public INewUI3DRenderObj
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

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y);
        void Release();

        void SetPos(int x, int y);

        // Called once when the window becomes visible: resets to the Browse tab and sends the open request.
        void OpeningProcess();
        // Called once when the window is hidden: closes the inventory window if this window is the one that
        // opened it for the Sell tab (not implemented until the Sell tab exists).
        void ClosingProcess();

        // Called from the network dispatch (WSclient.cpp) when an AuctionOpenResponse arrives. Ignored if its
        // RequestId does not match the currently pending open request (a stale or unexpected reply).
        void SetOpenResponse(const AuctionHouse::AuctionOpenResponse& response);

        // Called from the network dispatch when an AuctionBrowseResponse arrives. Ignored if its RequestId
        // does not match the currently pending browse request.
        void SetBrowseResponse(const AuctionHouse::AuctionBrowseResponse& response);

        bool Render() override;
        void Render3D() override;
        bool IsVisible() const override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override;

    private:
        void LoadImages();
        void UnloadImages();
        void RenderFrame();
        bool BtnProcess();
        void SendBrowseRequest();
        void RenderBrowseTab();
        void RebuildRowItems();
        void ReleaseRowItems();
        void RenderRowItemTooltip(int row) const;
        static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);

        CNewUIManager* m_pNewUIMng;
        CNewUI3DRenderMng* m_pNewUI3DRenderMng;
        POINT m_Pos;

        CNewUIRadioGroupButton m_TabBtn;
        int m_iCurrentTab;

        CNewUIButton m_BtnClose;

        bool m_bOpenRequestPending;
        uint32_t m_PendingOpenRequestId;
        bool m_bHasOpenResponse;
        AuctionHouse::AuctionOpenResponse m_OpenResponse;

        // Synced from whichever response arrives most recently (Open or Browse both carry ServerTime), so the
        // Browse tab's countdown can tick down locally between responses instead of only updating on the next
        // reply.
        AuctionHouse::AuctionServerClock m_ServerClock;

        // The Browse tab's selected currency, page and sort. V1 scope: the server only honors currency,
        // category, sort and page (confirmed by reading AuctionBrowseHandlerPlugIn.cs server-side); the
        // remaining wire fields (name search, class mask, level range, option flags, price range, max
        // remaining time) are sent as "no filter" and are not exposed as UI controls yet.
        AuctionCurrencyMode m_SelectedCurrency;
        uint16_t m_CurrentPage;
        AuctionSort m_SelectedSort;
        bool m_bBrowseRequestPending;
        uint32_t m_PendingBrowseRequestId;
        bool m_bHasBrowseResponse;
        AuctionHouse::AuctionBrowseResponse m_BrowseResponse;

        // At most this many rows are ever rendered or hit-tested at once (RenderBrowseTab's own constant, which
        // this array and the hover/icon logic must agree with).
        static constexpr size_t MaxBrowseRows = 8;

        // One owned ITEM* per rendered row, created from that row's AuctionListingSummary::ItemData via
        // g_pNewItemMng (the same manager the rest of NewUI uses), so the Browse tab can reuse RenderItem3D and
        // RenderItemInfo exactly as the inventory window does. Null for a row with no listing or no item data.
        // Rebuilt whenever m_BrowseResponse is replaced; always released through g_pNewItemMng, never deleted
        // directly, since CNewUIItemMng ref-counts and owns the underlying allocation.
        ITEM* m_RowItems[MaxBrowseRows];

        // The Browse row currently under the mouse, or -1. Computed in Update() from the same row geometry
        // RenderBrowseTab() and Render3D() use, so hover, icon, and tooltip never disagree about which row the
        // pointer is over.
        int m_iPointedRow;

        // One label per AuctionCurrencyMode value, in wire order, so GetSelectedIndex() doubles as the mode's
        // wire value. CNewUIComboBox keeps only the pointers, not a locale-change "slot" like CNewUIButton
        // does, so these labels will not refresh if the player changes language while this window is open —
        // a known limitation of this widget, not fixed here.
        const wchar_t* m_CurrencyLabels[9];
        CNewUIComboBox m_CurrencyCombo;
    };
}
