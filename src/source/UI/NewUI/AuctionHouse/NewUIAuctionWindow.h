// NewUIAuctionWindow.h: interface for the CNewUIAuctionWindow class.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Widgets/NewUIComboBox.h"
#include "UI/NewUI/Widgets/NewUIScrollBar.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/Legacy/UIControls.h"
#include "Guild/NewUIGuildInfoWindow.h"
#include "UI/NewUI/AuctionHouse/AuctionWireResponses.h"

namespace SEASON3B
{
    // The standalone Auction House window. Browse, My Listings, Mailbox collection, and Sell item selection
    // are implemented incrementally; the remaining Sell form controls are added in focused slices.
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
            IMAGE_AUCTION_PAGE_BTN = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL, // newui_btn_empty_very_small.tga
        };

        // The original 400-pixel design could not fit the owner-approved search/filter bar and six-column
        // results table. At x=20, 600 logical pixels preserve a 20-pixel margin on the 640-wide base canvas.
        static constexpr int WINDOW_WIDTH = 600;
        static constexpr int WINDOW_HEIGHT = 429;

        CNewUIAuctionWindow();
        ~CNewUIAuctionWindow() override;

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y);
        void Release();

        void SetPos(int x, int y);

        // Called once when the window becomes visible: resets to the Browse tab and sends the open request.
        void OpeningProcess();
        // Called once when the window is hidden: closes the inventory window if this window opened it for Sell.
        void ClosingProcess();

        // Called from the network dispatch (WSclient.cpp) when an AuctionOpenResponse arrives. Ignored if its
        // RequestId does not match the currently pending open request (a stale or unexpected reply).
        void SetOpenResponse(const AuctionHouse::AuctionOpenResponse& response);

        // Called from the network dispatch when an AuctionBrowseResponse arrives. Ignored if its RequestId
        // does not match the currently pending browse request.
        void SetBrowseResponse(const AuctionHouse::AuctionBrowseResponse& response);

        // Called for the account's owned-listing page. The wire layout is identical to Browse, but request
        // correlation is independent so a late response from another tab cannot replace the visible rows.
        void SetMyListingsResponse(const AuctionHouse::AuctionMyListingsResponse& response);

        // Called for the account-wide durable collection page. Correlated independently from listing queries
        // so a late Mailbox response cannot replace another tab's rows.
        void SetMailboxResponse(const AuctionHouse::AuctionMailboxResponse& response);

        // Real-time best-effort notification. Durable Mailbox state remains authoritative; this updates the
        // badge/message immediately and refreshes a visible Mailbox page.
        void SetNotification(const AuctionHouse::AuctionNotification& notification);

        // Called from the network dispatch when an AuctionDetailResponse arrives. Ignored if its RequestId
        // does not match the currently pending detail request.
        void SetDetailResponse(const AuctionHouse::AuctionDetailResponse& response);

        // Called from the network dispatch when an AuctionOperationResponse (bid, buyout, or cancellation) arrives.
        // Ignored if its OperationId does not match the currently pending mutation.
        void SetOperationResponse(const AuctionHouse::AuctionOperationResponse& response);

        // Invoked only by the modal confirmation's OK callback. Re-checks the current detail snapshot before
        // sending, because the listing can change between opening the confirmation and accepting it.
        void ConfirmCancelListing();

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
        int CurrentWindowWidth() const;
        void RenderFrame();
        bool BtnProcess();
        void SendBrowseRequest();
        void SendMyListingsRequest();
        void SendMailboxRequest();
        void SendCurrentPageRequest();
        void SubmitSearch();
        void RenderBrowseHeader();
        void RenderBrowseTab();
        void RenderCategoryRail();
        void RenderCurrencyBalances();
        void RenderMailboxHeader();
        void RenderMailboxTab();
        void EnterSellTab();
        void LeaveSellTab();
        void CaptureSellSelection();
        void ValidateSellSelection();
        void RenderSellTab();
        void RenderSellItemTooltip() const;
        void RebuildRowItems();
        void ReleaseRowItems();
        void RenderRowItemTooltip(int row) const;
        static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);
        void InitPageButton(CNewUIButton* pButton, int x, int y, const wchar_t* caption);
        void RenderPageControls();
        void SendDetailRequest(uint64_t listingId);
        void RebuildDetailItem();
        void ReleaseDetailItem();
        void RenderDetailPanel();
        void RenderMailboxDetailPanel();
        void PopulateCollectionClaimInputs();
        void SetCollectionClaimInputsVisible(bool visible);
        bool TryGetRequestedCollectionAmount(AuctionHouse::AuctionAmount& amount);
        bool HasCollectionClaimInputFocus();
        void RenderDetailItemTooltip() const;
        void SendBidRequest();
        void SendBuyoutRequest();
        void RequestCancelConfirmation();
        void SendCollectRequest();
        void RenderOperationButtons();
        void RepositionChildren();

        CNewUIManager* m_pNewUIMng;
        CNewUI3DRenderMng* m_pNewUI3DRenderMng;
        POINT m_Pos;

        // Drag-by-title-bar, the same mechanism CNewUIExpHuntWindow (the /hunt feature) already established
        // for a movable NewUI window: grab anywhere in the title strip (excluding the close button), hold,
        // release to drop. Unlike ExpHuntWindow (which has no interior widgets), this window is full of
        // buttons/combos/rows, so every child's absolute position has to be reapplied on every drag-move frame
        // via RepositionChildren() — nothing else in NewUI does this today because nothing else here moves.
        bool m_bDragging;
        int m_iDragGrabOffsetX;
        int m_iDragGrabOffsetY;

        CNewUIRadioGroupButton m_TabBtn;
        int m_iCurrentTab;

        CNewUIButton m_BtnClose;

        bool m_bOpenRequestPending;
        uint32_t m_PendingOpenRequestId;
        bool m_bHasOpenResponse;
        AuctionHouse::AuctionOpenResponse m_OpenResponse;

        // Sell owns the inventory only when it opened a previously hidden window. A selected lot is a value
        // snapshot plus its authoritative source slot; restoring the picked item immediately avoids moving it.
        bool m_bOpenedInventoryForSell;
        bool m_bHasSellItem;
        BYTE m_SellInventorySlot;
        ITEM m_SellItemSnapshot;
        bool m_bPointingSellItem;

        // Synced from whichever response arrives most recently (Open or Browse both carry ServerTime), so the
        // Browse tab's countdown can tick down locally between responses instead of only updating on the next
        // reply.
        AuctionHouse::AuctionServerClock m_ServerClock;

        // The Browse tab's selected currency, page and sort. The server now honors every filter field in the
        // request packet (confirmed by reading AuctionBrowseHandlerPlugIn.cs/AuctionHouseRepository.cs after
        // the 2026-10-07 browse-filter work). Name search has a toolbar control; class mask, level range,
        // option flags, price range, and max remaining time are still sent as "no filter" because they have no
        // UI control yet, not because the server would ignore them.
        AuctionCurrencyMode m_SelectedCurrency;
        // 0 = "All Categories"; 1-7 map to AuctionCategory's seven wire values (index - 1).
        int m_SelectedCategoryIndex;
        uint16_t m_CurrentPage;
        AuctionSort m_SelectedSort;
        bool m_bBrowseRequestPending;
        uint32_t m_PendingBrowseRequestId;
        bool m_bHasListingResponse;
        AuctionHouse::AuctionBrowseResponse m_ListingResponse;

        int m_SelectedStatusIndex;
        bool m_bMyListingsRequestPending;
        uint32_t m_PendingMyListingsRequestId;

        // Mailbox queries and rows have their own response state. The page number and viewport scrollbar are
        // shared with the other tabs because only one tab can be visible at a time.
        int m_SelectedCollectionKindIndex;
        bool m_bMailboxRequestPending;
        uint32_t m_PendingMailboxRequestId;
        bool m_bHasMailboxResponse;
        AuctionHouse::AuctionMailboxResponse m_MailboxResponse;
        uint16_t m_PendingMailboxCount;
        bool m_bHasSelectedCollection;
        AuctionHouse::AuctionMailboxEntry m_SelectedCollection;

        // Prev/Next page buttons for the Browse tab. Locked (and grayed) at page 1 and at the last known page;
        // clicking either re-sends the browse request for m_CurrentPage, same as a currency change does.
        CNewUIButton m_BtnPrevPage;
        CNewUIButton m_BtnNextPage;

        // At most this many rows are ever rendered or hit-tested at once (RenderBrowseTab's own constant, which
        // this array and the hover/icon logic must agree with).
        static constexpr size_t MaxBrowseRows = 8;

        // The server returns up to three viewports per page. This widget scrolls within that received page;
        // Prev/Next still move between server pages when more than 24 matching listings exist.
        CNewUIScrollBar m_BrowseScrollBar;
        size_t m_BrowseScrollOffset;

        // One owned ITEM* per rendered row, created from that row's AuctionListingSummary::ItemData via
        // g_pNewItemMng (the same manager the rest of NewUI uses), so the Browse tab can reuse RenderItem3D and
        // RenderItemInfo exactly as the inventory window does. Null for a row with no listing or no item data.
        // Rebuilt whenever the current Browse/My Listings page is replaced; always released through
        // g_pNewItemMng, never deleted
        // directly, since CNewUIItemMng ref-counts and owns the underlying allocation.
        ITEM* m_RowItems[MaxBrowseRows];

        // The Browse row currently under the mouse, or -1. Computed in Update() from the same row geometry
        // RenderBrowseTab() and Render3D() use, so hover, icon, and tooltip never disagree about which row the
        // pointer is over.
        int m_iPointedRow;

        // The tooltip has a tighter hit target than a row selection: it is shown only over the item-icon cell.
        int m_iPointedItemRow;

        // Index zero is the all-currencies sentinel, then one label per AuctionCurrencyMode value in wire
        // order. CNewUIComboBox keeps only the pointers, not a locale-change "slot" like CNewUIButton
        // does, so these labels will not refresh if the player changes language while this window is open —
        // a known limitation of this widget, not fixed here.
        const wchar_t* m_CurrencyLabels[10];
        CNewUIComboBox m_CurrencyCombo;

        // Index 0 is "All Categories" (sent on the wire as the server's own 0xFF "every category" sentinel);
        // indices 1-7 are AuctionCategory's seven wire values in order, so GetSelectedIndex() - 1 is the wire
        // category value whenever a specific category is selected.
        const wchar_t* m_CategoryLabels[8];
        CNewUIComboBox m_CategoryCombo;

        // All statuses followed by Active/Sold/Expired/Cancelled/Admin Removed, matching the status-filter
        // mapping tested in AuctionModel.
        const wchar_t* m_StatusLabels[6];
        CNewUIComboBox m_StatusCombo;

        // All collections followed by AuctionCollectionKind's six wire values in declaration order.
        const wchar_t* m_CollectionKindLabels[7];
        CNewUIComboBox m_CollectionKindCombo;

        // Single-line UTF-16 edit field converted to the browse packet's fixed 32-byte UTF-8 name field on
        // submit. The adjacent Search button and Enter both call SubmitSearch().
        CUITextInputBox m_SearchInput;
        // The browse packet has supported this range from the first implementation. Keep its controls beside
        // the item search so the visible filter bar matches the actual server query rather than a mock-up.
        CUITextInputBox m_MinimumLevelInput;
        CUITextInputBox m_MaximumLevelInput;
        CNewUIButton m_BtnSearch;
        CNewUIButton m_BtnReset;

        // Selected-listing detail (design spec 4.2's main body swaps between "rows" and "detail panel"). True
        // while the detail panel replaces the row list; cleared on any tab switch, Back, or window re-open.
        bool m_bShowingDetail;
        CNewUIButton m_BtnBack;
        bool m_bDetailRequestPending;
        uint32_t m_PendingDetailRequestId;
        bool m_bHasDetailResponse;
        AuctionHouse::AuctionDetailResponse m_DetailResponse;

        // Owned separately from m_RowItems: the detail panel shows exactly one item, rebuilt from the detail
        // response's own ItemData snapshot rather than reusing whichever row happened to be clicked, so it stays
        // correct even if the underlying Browse page is refreshed while the panel is open.
        ITEM* m_DetailItem;
        bool m_bPointingDetailItem;

        // Bid/Buyout mutation state. Correlated by the 16-byte OperationId (not a RequestId, unlike every
        // query above), matching the design spec's own AuctionOperationResponse shape. ComputeMinimumNextBid
        // is advisory client-side guidance for the quick-bid button; the server independently re-validates.
        CNewUIButton m_BtnBid;
        CNewUIButton m_BtnBuyout;
        CNewUIButton m_BtnCancelListing;
        CNewUIButton m_BtnCollect;
        // One numeric field for jewel claims, or five fields for a fruit vector. Item and Zen collection stay
        // all-or-nothing and therefore show no quantity input.
        CUITextInputBox m_CollectionClaimInputs[5];
        bool m_bOperationRequestPending;
        std::array<uint8_t, 16> m_PendingOperationId;
        bool m_bHasOperationResult;
        AuctionResult m_LastOperationResult;
        std::wstring m_LastOperationMessage;
    };
}
