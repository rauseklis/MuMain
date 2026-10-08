// NewUIAuctionWindow.cpp: implementation of the CNewUIAuctionWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Dialogs/NewUICommonMessageBox.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "Engine/Object/ZzzInventory.h"
#include "GameLogic/Items/InventoryUtils.h"
#include "I18N/All.h"
#include "Audio/DSPlaySound.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/SocketSystem.h"
#include "Core/Text/Utf8.h"

#include <algorithm>

using namespace SEASON3B;

namespace
{
    constexpr int TAB_REGION_X = 10;
    constexpr int TAB_REGION_Y = 40;
    // Four equal tabs span the wider window while preserving the existing 10-pixel side margins.
    constexpr int TAB_WIDTH = (SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - 2 * TAB_REGION_X) / 4;
    constexpr int TAB_HEIGHT = 26;
    constexpr int TOP_BAND_HEIGHT = 64;
    constexpr int BOTTOM_BAND_HEIGHT = 45;
    constexpr int SIDE_BAND_WIDTH = 21;
    constexpr int FRAME_TEXTURE_WIDTH = 190;
    constexpr int FRAME_TEXTURE_HEIGHT = 429;
    constexpr int FRAME_CAP_WIDTH = 64;
    constexpr int FRAME_CENTER_WIDTH = FRAME_TEXTURE_WIDTH - 2 * FRAME_CAP_WIDTH;
    constexpr int CLOSE_BTN_WIDTH = 36;
    constexpr int CLOSE_BTN_HEIGHT = 29;
    constexpr int CLOSE_BTN_MARGIN = 8;
    constexpr int TOOLBAR_X = 11;
    constexpr int TOOLBAR_Y = 69;
    constexpr int CURRENCY_COMBO_WIDTH = 105;
    constexpr int CURRENCY_COMBO_ITEM_HEIGHT = 22;
    constexpr int CATEGORY_COMBO_X_OFFSET = CURRENCY_COMBO_WIDTH + 8;
    constexpr int CATEGORY_COMBO_WIDTH = 210;
    constexpr int BROWSE_TABLE_X = 132;
    constexpr int SEARCH_INPUT_X_OFFSET = BROWSE_TABLE_X + CURRENCY_COMBO_WIDTH + 8 - TOOLBAR_X;
    constexpr int SEARCH_INPUT_Y_OFFSET = TOOLBAR_Y + 3;
    constexpr int SEARCH_INPUT_HEIGHT = 16;
    constexpr int SEARCH_BUTTON_WIDTH = 53;
    constexpr int SEARCH_BUTTON_X_OFFSET = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - 2 * TOOLBAR_X - SEARCH_BUTTON_WIDTH;
    constexpr int SEARCH_INPUT_WIDTH = SEARCH_BUTTON_X_OFFSET - SEARCH_INPUT_X_OFFSET - 7;
    constexpr size_t SEARCH_PACKET_CAPACITY = 32;
    static_assert(SEARCH_INPUT_WIDTH > 0, "Auction search controls exceed the toolbar width");
    constexpr int BROWSE_BODY_X = 11;
    constexpr int BROWSE_HEADER_Y = 105;
    constexpr int BROWSE_HEADER_HEIGHT = 21;
    constexpr int BROWSE_BODY_Y = BROWSE_HEADER_Y + BROWSE_HEADER_HEIGHT;
    constexpr int BROWSE_ROW_HEIGHT = 29;
    constexpr int BROWSE_ROW_PADDING = 1;
    constexpr int BROWSE_ICON_SIZE = 24;
    constexpr int BROWSE_ICON_MARGIN = 2;
    constexpr int BROWSE_TEXT_X_OFFSET = BROWSE_ICON_SIZE + 6 + BROWSE_ICON_MARGIN;
    constexpr int BROWSE_SCROLLBAR_RIGHT_MARGIN = 14;
    constexpr int BROWSE_SCROLLBAR_HEIGHT = BROWSE_ROW_HEIGHT * 8;
    constexpr int BROWSE_SCROLLBAR_RESERVED_WIDTH = 18;
    constexpr int BROWSE_CONTENT_WIDTH = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - 2 * BROWSE_BODY_X - BROWSE_SCROLLBAR_RESERVED_WIDTH;
    constexpr int BROWSE_TABLE_CONTENT_WIDTH = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - BROWSE_TABLE_X - BROWSE_BODY_X - BROWSE_SCROLLBAR_RESERVED_WIDTH;
    constexpr int BROWSE_ITEM_COLUMN_X = 0;
    constexpr int BROWSE_ITEM_COLUMN_WIDTH = 164;
    constexpr int BROWSE_LEVEL_COLUMN_X = BROWSE_ITEM_COLUMN_X + BROWSE_ITEM_COLUMN_WIDTH;
    constexpr int BROWSE_LEVEL_COLUMN_WIDTH = 36;
    constexpr int BROWSE_TIME_COLUMN_X = BROWSE_LEVEL_COLUMN_X + BROWSE_LEVEL_COLUMN_WIDTH;
    constexpr int BROWSE_TIME_COLUMN_WIDTH = 58;
    constexpr int BROWSE_SELLER_COLUMN_X = BROWSE_TIME_COLUMN_X + BROWSE_TIME_COLUMN_WIDTH;
    constexpr int BROWSE_SELLER_COLUMN_WIDTH = 60;
    constexpr int BROWSE_PRICE_COLUMN_X = BROWSE_SELLER_COLUMN_X + BROWSE_SELLER_COLUMN_WIDTH;
    constexpr int BROWSE_PRICE_COLUMN_WIDTH = 60;
    constexpr int BROWSE_BUYOUT_COLUMN_X = BROWSE_PRICE_COLUMN_X + BROWSE_PRICE_COLUMN_WIDTH;
    constexpr int BROWSE_BUYOUT_COLUMN_WIDTH = BROWSE_TABLE_CONTENT_WIDTH - BROWSE_BUYOUT_COLUMN_X;
    constexpr int CATEGORY_RAIL_X = BROWSE_BODY_X;
    constexpr int CATEGORY_RAIL_Y = BROWSE_HEADER_Y;
    constexpr int CATEGORY_RAIL_WIDTH = BROWSE_TABLE_X - BROWSE_BODY_X - 7;
    constexpr int CATEGORY_RAIL_ROW_HEIGHT = 29;
    constexpr int BALANCE_BAR_X = BROWSE_TABLE_X;
    constexpr int BALANCE_BAR_Y = BROWSE_BODY_Y + BROWSE_SCROLLBAR_HEIGHT + 4;
    constexpr int BALANCE_BAR_WIDTH = BROWSE_TABLE_CONTENT_WIDTH;
    constexpr int BALANCE_BAR_HEIGHT = 21;
    constexpr int BALANCE_ICON_SIZE = 17;
    constexpr int MAILBOX_KIND_COLUMN_WIDTH = 145;
    constexpr int MAILBOX_CONTENT_COLUMN_X = MAILBOX_KIND_COLUMN_WIDTH;
    constexpr int MAILBOX_CONTENT_COLUMN_WIDTH = 245;
    constexpr int MAILBOX_SOURCE_COLUMN_X = MAILBOX_CONTENT_COLUMN_X + MAILBOX_CONTENT_COLUMN_WIDTH;
    constexpr int MAILBOX_SOURCE_COLUMN_WIDTH = 80;
    constexpr int MAILBOX_STATUS_COLUMN_X = MAILBOX_SOURCE_COLUMN_X + MAILBOX_SOURCE_COLUMN_WIDTH;
    constexpr int MAILBOX_STATUS_COLUMN_WIDTH = BROWSE_CONTENT_WIDTH - MAILBOX_STATUS_COLUMN_X;
    static_assert(BROWSE_BUYOUT_COLUMN_WIDTH > 0, "Auction browse columns exceed their content region");
    // A visible panel behind every row, so each listing reads as a distinct card rather than bare text
    // floating on the window background — the same idea as WoW's Auction House row cards, built from plain
    // colored quads since no card-panel texture exists in this project's asset set.
    constexpr unsigned int BROWSE_CARD_COLOR = 0x30FFFFFFu;
    constexpr unsigned int BROWSE_CARD_HOVER_COLOR = 0x50FFD700u;
    constexpr unsigned int BROWSE_HEADER_COLOR = 0xA018120Au;
    constexpr unsigned int BROWSE_GRID_COLOR = 0x50C8A45Au;
    constexpr int PAGE_BTN_WIDTH = 53;
    constexpr int PAGE_BTN_HEIGHT = 23;
    constexpr int PAGE_BTN_MARGIN_X = 20;
    constexpr int PAGE_BTN_Y_OFFSET = SEASON3B::CNewUIAuctionWindow::WINDOW_HEIGHT - BOTTOM_BAND_HEIGHT + (BOTTOM_BAND_HEIGHT - PAGE_BTN_HEIGHT) / 2;
    constexpr int DETAIL_ICON_SIZE = 32;
    constexpr int DETAIL_TEXT_X_OFFSET = DETAIL_ICON_SIZE + 10;
    constexpr int DETAIL_LINE_HEIGHT = 18;
    constexpr int BACK_BTN_WIDTH = 53;
    constexpr int BACK_BTN_HEIGHT = 23;
    constexpr int OPERATION_RESULT_Y_OFFSET = PAGE_BTN_Y_OFFSET - DETAIL_LINE_HEIGHT - 2;
    constexpr int CLAIM_INPUT_Y_OFFSET = BROWSE_BODY_Y + 108;
    constexpr int CLAIM_INPUT_WIDTH = 58;
    constexpr int CLAIM_INPUT_HEIGHT = 16;
    constexpr int CLAIM_INPUT_COLUMN_WIDTH = 105;
    constexpr int SELL_PANEL_X = 20;
    constexpr int SELL_PANEL_Y = 112;
    constexpr int SELL_WINDOW_WIDTH = 400;
    constexpr int SELL_PANEL_WIDTH = SELL_WINDOW_WIDTH - 2 * SELL_PANEL_X;
    constexpr int SELL_PANEL_HEIGHT = 92;
    constexpr int SELL_ICON_SIZE = 52;
    constexpr int SELL_ICON_MARGIN = 12;

    const wchar_t* CollectionKindText(AuctionCollectionKind kind)
    {
        switch (kind)
        {
        case AuctionCollectionKind::PurchasedItem: return I18N::Game::AuctionPurchasedItem;
        case AuctionCollectionKind::ReturnedItem: return I18N::Game::AuctionReturnedItem;
        case AuctionCollectionKind::SaleProceeds: return I18N::Game::AuctionSaleProceeds;
        case AuctionCollectionKind::OutbidRefund: return I18N::Game::AuctionOutbidRefund;
        case AuctionCollectionKind::TenderChange: return I18N::Game::AuctionTenderChange;
        case AuctionCollectionKind::AdminRefund: return I18N::Game::AuctionAdminRefund;
        default: return L"-";
        }
    }

    const wchar_t* CollectionStatusText(AuctionCollectionStatus status)
    {
        switch (status)
        {
        case AuctionCollectionStatus::Pending: return I18N::Game::AuctionPending;
        case AuctionCollectionStatus::PartiallyClaimed: return I18N::Game::AuctionPartiallyClaimed;
        case AuctionCollectionStatus::Claimed: return I18N::Game::AuctionClaimed;
        default: return L"-";
        }
    }

    std::wstring AuctionAmountText(AuctionCurrencyMode currency, const AuctionHouse::AuctionAmount& amount)
    {
        if (amount.IsFruitBasket())
        {
            const auto& f = amount.Fruits();
            wchar_t text[128];
            mu_swprintf(text, L"Str %lld  Agi %lld  Vit %lld  Ene %lld  Cmd %lld",
                f.Strength, f.Agility, f.Vitality, f.Energy, f.Command);
            return text;
        }

        static const wchar_t* names[] = { L"Zen", L"Chaos", L"Bless", L"Soul", L"Life", L"Creation", L"Guardian", L"Harmony" };
        const auto index = static_cast<size_t>(currency);
        const wchar_t* name = index < std::size(names) ? names[index] : L"Units";
        return std::to_wstring(amount.Scalar()) + L" " + name;
    }

    void SetBrowseItemNameColor(const ITEM* item)
    {
        if (item == nullptr)
        {
            g_pRenderText->SetTextColor(255, 255, 255, 255);
        }
        else if (item->AncientDiscriminator > 0)
        {
            // Matches RenderItemInfo's ancient-name foreground (its blue tooltip background is not suitable
            // inside a table cell).
            g_pRenderText->SetTextColor(0, 255, 0, 255);
        }
        else if (g_SocketItemMgr.IsSocketItem(item))
        {
            g_pRenderText->SetTextColor(179, 102, 255, 255);
        }
        else if (item->SpecialNum > 0 && item->ExcellentFlags > 0)
        {
            g_pRenderText->SetTextColor(26, 255, 128, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(255, 255, 255, 255);
        }
    }

    const wchar_t* ListingStatusText(AuctionListingStatus status)
    {
        switch (status)
        {
        case AuctionListingStatus::Active: return I18N::Game::AuctionActive;
        case AuctionListingStatus::Sold: return I18N::Game::AuctionSold;
        case AuctionListingStatus::Expired: return I18N::Game::AuctionExpired;
        case AuctionListingStatus::Cancelled: return I18N::Game::Cancelled;
        case AuctionListingStatus::AdminRemoved: return I18N::Game::AuctionAdminRemoved;
        default: return L"-";
        }
    }

    class AuctionCancelMsgBoxLayout : public SEASON3B::TMsgBoxLayout<SEASON3B::CNewUICommonMessageBox>
    {
    public:
        bool SetLayout()
        {
            auto* messageBox = GetMsgBox();
            if (messageBox == nullptr || !messageBox->Create(SEASON3B::MSGBOX_COMMON_TYPE_OKCANCEL))
            {
                return false;
            }

            messageBox->AddMsg(I18N::Game::AuctionCancelConfirmation);
            messageBox->AddCallbackFunc(OkBtnDown, MSGBOX_EVENT_USER_COMMON_OK);
            messageBox->AddCallbackFunc(CancelBtnDown, MSGBOX_EVENT_USER_COMMON_CANCEL);
            messageBox->AddCallbackFunc(OkBtnDown, MSGBOX_EVENT_PRESSKEY_RETURN);
            messageBox->AddCallbackFunc(CancelBtnDown, MSGBOX_EVENT_PRESSKEY_ESC);
            return true;
        }

        static CALLBACK_RESULT OkBtnDown(SEASON3B::CNewUIMessageBoxBase* owner, const leaf::xstreambuf&)
        {
            if (g_pAuctionWindow != nullptr)
            {
                g_pAuctionWindow->ConfirmCancelListing();
            }
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(owner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }

        static CALLBACK_RESULT CancelBtnDown(SEASON3B::CNewUIMessageBoxBase* owner, const leaf::xstreambuf&)
        {
            PlayBuffer(SOUND_CLICK01);
            g_MessageBox->SendEvent(owner, MSGBOX_EVENT_DESTROY);
            return CALLBACK_BREAK;
        }
    };
}

SEASON3B::CNewUIAuctionWindow::CNewUIAuctionWindow()
    : m_pNewUIMng(nullptr), m_pNewUI3DRenderMng(nullptr), m_Pos{ 0, 0 },
      m_bDragging(false), m_iDragGrabOffsetX(0), m_iDragGrabOffsetY(0), m_iCurrentTab(TAB_BROWSE),
      m_bOpenRequestPending(false), m_PendingOpenRequestId(0), m_bHasOpenResponse(false),
      m_bOpenedInventoryForSell(false), m_bHasSellItem(false), m_SellInventorySlot(0),
      m_SellItemSnapshot{}, m_bPointingSellItem(false),
      m_SelectedCurrency(AuctionCurrencyMode::Zen), m_SelectedCategoryIndex(0), m_CurrentPage(1), m_SelectedSort(AuctionSort::EndingSoonest),
      m_bBrowseRequestPending(false), m_PendingBrowseRequestId(0), m_bHasListingResponse(false),
      m_SelectedStatusIndex(0), m_bMyListingsRequestPending(false), m_PendingMyListingsRequestId(0),
      m_SelectedCollectionKindIndex(0), m_bMailboxRequestPending(false), m_PendingMailboxRequestId(0),
      m_bHasMailboxResponse(false), m_PendingMailboxCount(0), m_bHasSelectedCollection(false),
      m_BrowseScrollOffset(0), m_iPointedRow(-1),
      m_bShowingDetail(false), m_bDetailRequestPending(false), m_PendingDetailRequestId(0),
      m_bHasDetailResponse(false), m_DetailItem(nullptr), m_bPointingDetailItem(false),
      m_bOperationRequestPending(false), m_PendingOperationId{}, m_bHasOperationResult(false),
      m_LastOperationResult(AuctionResult::Success)
{
    std::fill(std::begin(m_RowItems), std::end(m_RowItems), nullptr);
}

SEASON3B::CNewUIAuctionWindow::~CNewUIAuctionWindow()
{
    Release();
}

bool SEASON3B::CNewUIAuctionWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y)
{
    if (nullptr == pNewUIMng || nullptr == pNewUI3DRenderMng)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_AUCTION_HOUSE, this);

    m_pNewUI3DRenderMng = pNewUI3DRenderMng;
    // The Auction window itself renders above the inventory camera. Queue its item models on the
    // information camera so the opaque Auction frame cannot cover them.
    m_pNewUI3DRenderMng->Add3DRenderObj(this, INFORMATION_CAMERA_Z_ORDER);

    SetPos(x, y);
    LoadImages();

    std::list<const wchar_t* const*> tabLabels;
    tabLabels.push_back(&I18N::Game::Browse);
    tabLabels.push_back(&I18N::Game::Sell);
    tabLabels.push_back(&I18N::Game::MyListings);
    tabLabels.push_back(&I18N::Game::Mailbox);

    m_TabBtn.CreateRadioGroup(TAB_COUNT, IMAGE_AUCTION_TAB_BTN);
    m_TabBtn.ChangeRadioText(tabLabels);
    m_TabBtn.ChangeRadioButtonInfo(true, (float)(m_Pos.x + TAB_REGION_X), (float)(m_Pos.y + TAB_REGION_Y), TAB_WIDTH, TAB_HEIGHT);
    m_TabBtn.ChangeFrame(m_iCurrentTab);

    m_BtnClose.ChangeButtonImgState(true, IMAGE_AUCTION_CLOSE_BTN, false);
    m_BtnClose.ChangeButtonInfo(m_Pos.x + WINDOW_WIDTH - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN, m_Pos.y + CLOSE_BTN_MARGIN, CLOSE_BTN_WIDTH, CLOSE_BTN_HEIGHT);
    m_BtnClose.ChangeToolTipText(&I18N::Game::Close388);

    // Labels in AuctionCurrencyMode wire order (0 Zen .. 8 Fruits), so the combo's selected index is the
    // currency's wire value directly, no lookup table needed.
    m_CurrencyLabels[0] = I18N::Game::Zen;
    m_CurrencyLabels[1] = I18N::Game::JewelOfChaos;
    m_CurrencyLabels[2] = I18N::Game::JewelOfBless;
    m_CurrencyLabels[3] = I18N::Game::JewelOfSoul;
    m_CurrencyLabels[4] = I18N::Game::JewelOfLife;
    m_CurrencyLabels[5] = I18N::Game::JewelOfCreation;
    m_CurrencyLabels[6] = I18N::Game::JewelOfGuardian;
    m_CurrencyLabels[7] = I18N::Game::JewelOfHarmony;
    m_CurrencyLabels[8] = I18N::Game::FruitBasket;
    m_CurrencyCombo.Setup(m_Pos.x + BROWSE_TABLE_X, m_Pos.y + TOOLBAR_Y, CURRENCY_COMBO_WIDTH, CURRENCY_COMBO_ITEM_HEIGHT,
        m_CurrencyLabels, 9, static_cast<int>(m_SelectedCurrency));

    // Index 0 is "All Categories" (the server's own 0xFF sentinel); indices 1-7 are AuctionCategory's seven
    // wire values in declaration order (Weapon, Armor, Wing, Pet & Helper, Jewel & Material, Consumable,
    // Miscellaneous) — see AuctionCategory.cs server-side.
    m_CategoryLabels[0] = I18N::Game::AllCategories;
    m_CategoryLabels[1] = I18N::Game::Weapon;
    m_CategoryLabels[2] = I18N::Game::Armor;
    m_CategoryLabels[3] = I18N::Game::Wing;
    m_CategoryLabels[4] = I18N::Game::PetHelper;
    m_CategoryLabels[5] = I18N::Game::JewelMaterial;
    m_CategoryLabels[6] = I18N::Game::Consumable;
    m_CategoryLabels[7] = I18N::Game::Miscellaneous;
    m_CategoryCombo.Setup(m_Pos.x + TOOLBAR_X + CATEGORY_COMBO_X_OFFSET, m_Pos.y + TOOLBAR_Y, CATEGORY_COMBO_WIDTH, CURRENCY_COMBO_ITEM_HEIGHT,
        m_CategoryLabels, 8, m_SelectedCategoryIndex);

    m_StatusLabels[0] = I18N::Game::AuctionAllStatuses;
    m_StatusLabels[1] = I18N::Game::AuctionActive;
    m_StatusLabels[2] = I18N::Game::AuctionSold;
    m_StatusLabels[3] = I18N::Game::AuctionExpired;
    m_StatusLabels[4] = I18N::Game::Cancelled;
    m_StatusLabels[5] = I18N::Game::AuctionAdminRemoved;
    m_StatusCombo.Setup(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, CATEGORY_COMBO_WIDTH,
        CURRENCY_COMBO_ITEM_HEIGHT, m_StatusLabels, 6, m_SelectedStatusIndex);

    m_CollectionKindLabels[0] = I18N::Game::AuctionAllCollections;
    m_CollectionKindLabels[1] = I18N::Game::AuctionPurchasedItem;
    m_CollectionKindLabels[2] = I18N::Game::AuctionReturnedItem;
    m_CollectionKindLabels[3] = I18N::Game::AuctionSaleProceeds;
    m_CollectionKindLabels[4] = I18N::Game::AuctionOutbidRefund;
    m_CollectionKindLabels[5] = I18N::Game::AuctionTenderChange;
    m_CollectionKindLabels[6] = I18N::Game::AuctionAdminRefund;
    m_CollectionKindCombo.Setup(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, CATEGORY_COMBO_WIDTH,
        CURRENCY_COMBO_ITEM_HEIGHT, m_CollectionKindLabels, 7, m_SelectedCollectionKindIndex);

    m_SearchInput.Init(g_hWnd, SEARCH_INPUT_WIDTH, SEARCH_INPUT_HEIGHT, 32, false);
    m_SearchInput.SetPosition(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_SearchInput.SetTextColor(255, 255, 230, 210);
    m_SearchInput.SetBackColor(210, 20, 14, 8);
    m_SearchInput.SetSelectBackColor(255, 95, 68, 24);
    m_SearchInput.SetFont(g_hFont);
    m_SearchInput.SetParentUIID(SEASON3B::INTERFACE_AUCTION_HOUSE);
    m_SearchInput.SetState(UISTATE_NORMAL);

    for (size_t i = 0; i < std::size(m_CollectionClaimInputs); ++i)
    {
        auto& input = m_CollectionClaimInputs[i];
        input.Init(g_hWnd, CLAIM_INPUT_WIDTH, CLAIM_INPUT_HEIGHT, 10, false);
        input.SetPosition(m_Pos.x + TOOLBAR_X + static_cast<int>(i) * CLAIM_INPUT_COLUMN_WIDTH,
            m_Pos.y + CLAIM_INPUT_Y_OFFSET + 15);
        input.SetTextColor(255, 255, 230, 210);
        input.SetBackColor(210, 20, 14, 8);
        input.SetSelectBackColor(255, 95, 68, 24);
        input.SetFont(g_hFont);
        input.SetParentUIID(SEASON3B::INTERFACE_AUCTION_HOUSE);
        input.SetOption(UIOPTION_NUMBERONLY);
        input.SetState(UISTATE_HIDE);
    }

    InitPageButton(&m_BtnPrevPage, m_Pos.x + PAGE_BTN_MARGIN_X, m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Previous);
    InitPageButton(&m_BtnNextPage, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH, m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Next);
    InitPageButton(&m_BtnBack, m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, I18N::Game::Back);
    InitPageButton(&m_BtnBid, m_Pos.x + PAGE_BTN_MARGIN_X, m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Bid);
    InitPageButton(&m_BtnBuyout, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH, m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Buyout);
    InitPageButton(&m_BtnCancelListing, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Cancel);
    InitPageButton(&m_BtnCollect, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::AuctionCollect);
    InitPageButton(&m_BtnSearch, m_Pos.x + TOOLBAR_X + SEARCH_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, I18N::Game::AuctionSearch);
    m_BrowseScrollBar.Create(m_Pos.x + WINDOW_WIDTH - BROWSE_SCROLLBAR_RIGHT_MARGIN, m_Pos.y + BROWSE_BODY_Y, BROWSE_SCROLLBAR_HEIGHT);
    m_BrowseScrollBar.Show(false);

    Show(false);

    return true;
}

void SEASON3B::CNewUIAuctionWindow::Release()
{
    m_SearchInput.SetState(UISTATE_HIDE);
    SetCollectionClaimInputsVisible(false);
    if (m_SearchInput.HaveFocus())
    {
        CUITextInputBox::ReleaseFocus();
    }

    UnloadImages();
    ReleaseRowItems();
    ReleaseDetailItem();

    if (m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->Remove3DRenderObj(this);
        m_pNewUI3DRenderMng = nullptr;
    }

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIAuctionWindow::InitPageButton(CNewUIButton* pButton, int x, int y, const wchar_t* caption)
{
    pButton->ChangeText(caption);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeButtonImgState(true, IMAGE_AUCTION_PAGE_BTN, true);
    pButton->ChangeButtonInfo(x, y, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    pButton->ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    pButton->ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
}

void SEASON3B::CNewUIAuctionWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
}

void SEASON3B::CNewUIAuctionWindow::RepositionChildren()
{
    // Every child widget was given an absolute, one-time position in Create() (the established NewUI
    // convention, since nothing else here ever moves). Dragging breaks that assumption, so every widget's
    // position has to be reapplied against the new m_Pos on every drag-move frame.
    const int windowWidth = CurrentWindowWidth();
    const int tabWidth = (windowWidth - 2 * TAB_REGION_X) / 4;
    m_TabBtn.ChangeRadioButtonInfo(true, (float)(m_Pos.x + TAB_REGION_X), (float)(m_Pos.y + TAB_REGION_Y), tabWidth, TAB_HEIGHT);
    m_BtnClose.ChangeButtonInfo(m_Pos.x + windowWidth - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN, m_Pos.y + CLOSE_BTN_MARGIN, CLOSE_BTN_WIDTH, CLOSE_BTN_HEIGHT);
    m_CurrencyCombo.SetPos(m_Pos.x + BROWSE_TABLE_X, m_Pos.y + TOOLBAR_Y);
    m_CategoryCombo.SetPos(m_Pos.x + TOOLBAR_X + CATEGORY_COMBO_X_OFFSET, m_Pos.y + TOOLBAR_Y);
    m_StatusCombo.SetPos(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y);
    m_CollectionKindCombo.SetPos(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y);
    for (size_t i = 0; i < std::size(m_CollectionClaimInputs); ++i)
    {
        m_CollectionClaimInputs[i].SetPosition(
            m_Pos.x + TOOLBAR_X + static_cast<int>(i) * CLAIM_INPUT_COLUMN_WIDTH,
            m_Pos.y + CLAIM_INPUT_Y_OFFSET + 15);
    }
    m_SearchInput.SetPosition(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_BtnSearch.ChangeButtonInfo(m_Pos.x + TOOLBAR_X + SEARCH_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, SEARCH_BUTTON_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnPrevPage.ChangeButtonInfo(m_Pos.x + PAGE_BTN_MARGIN_X, m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnNextPage.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH, m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnBack.ChangeButtonInfo(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnBid.ChangeButtonInfo(m_Pos.x + PAGE_BTN_MARGIN_X, m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnBuyout.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH, m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnCancelListing.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnCollect.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BrowseScrollBar.SetPos(m_Pos.x + windowWidth - BROWSE_SCROLLBAR_RIGHT_MARGIN, m_Pos.y + BROWSE_BODY_Y);
    m_BrowseScrollBar.UpdateScrolling();
    if (m_iCurrentTab == TAB_SELL && m_bOpenedInventoryForSell)
    {
        g_pMyInventory->SetPos(m_Pos.x + windowWidth + 10, m_Pos.y);
    }
}

void SEASON3B::CNewUIAuctionWindow::OpeningProcess()
{
    // Reset to Browse every time the window opens, and ask the server for the current fees/currencies/
    // pending Mailbox count. The design spec's "loading state until the response arrives" (4.1) is UI that
    // belongs to the Browse tab (task 4.3), which does not exist yet; m_bHasOpenResponse tracks whether one
    // has arrived, for that tab to read once it does.
    m_iCurrentTab = TAB_BROWSE;
    m_TabBtn.ChangeFrame(m_iCurrentTab);
    SetLayoutMode(UI::Scaling::LayoutMode::Dialog);
    RepositionChildren();

    m_bShowingDetail = false;
    m_bHasSelectedCollection = false;
    SetCollectionClaimInputsVisible(false);
    ReleaseDetailItem();
    m_bHasOperationResult = false;
    m_LastOperationMessage.clear();
    m_bOperationRequestPending = false;
    m_bOpenedInventoryForSell = false;
    m_bHasSellItem = false;
    m_bPointingSellItem = false;
    m_SearchInput.SetText(L"");
    m_SearchInput.SetState(UISTATE_NORMAL);
    SetRelatedWnd(g_hWnd);

    m_bHasOpenResponse = false;
    m_PendingOpenRequestId = AuctionHouse::NextAuctionRequestId();
    m_bOpenRequestPending = true;
    SocketClient->ToGameServer()->SendAuctionOpenRequest(m_PendingOpenRequestId);

    m_CurrentPage = 1;
    m_BrowseScrollOffset = 0;
    m_BrowseScrollBar.SetCurPos(0);
    SendBrowseRequest();
}

void SEASON3B::CNewUIAuctionWindow::SendBrowseRequest()
{
    // The server now honors every field below (confirmed by reading AuctionBrowseHandlerPlugIn.cs/
    // AuctionHouseRepository.cs after the 2026-10-07 browse-filter work). Category 255 means "every category"
    // (the server's own sentinel, AuctionBrowseHandlerPlugIn.AllCategories); m_SelectedCategoryIndex == 0 maps
    // to that sentinel, otherwise it's (index - 1) as the wire AuctionCategory value. Name is taken from the
    // search box; the remaining fields are sent as the widest possible range because they have no UI control
    // yet, which the server reads as "no filter" rather than as a server-side limitation.
    constexpr BYTE AllCategories = 0xFF;
    const BYTE category = m_SelectedCategoryIndex == 0 ? AllCategories : static_cast<BYTE>(m_SelectedCategoryIndex - 1);
    wchar_t searchText[33]{};
    m_SearchInput.GetText(searchText, static_cast<int>(std::size(searchText)));
    const std::string searchUtf8 = AuctionHouse::ClampUtf8ToByteCapacity(Core::Text::ToUtf8(searchText), SEARCH_PACKET_CAPACITY);
    m_bHasListingResponse = false;
    m_BrowseScrollOffset = 0;
    m_BrowseScrollBar.SetCurPos(0);
    m_BrowseScrollBar.Show(false);
    ReleaseRowItems();
    m_PendingBrowseRequestId = AuctionHouse::NextAuctionRequestId();
    m_bBrowseRequestPending = true;
    SocketClient->ToGameServer()->SendAuctionBrowseRequest(
        m_PendingBrowseRequestId,
        m_CurrentPage,
        category,
        m_SelectedCurrency,
        m_SelectedSort,
        0xFFFFFFFFu, // class mask: every class
        0, // level minimum
        255, // level maximum
        0, // option flags: none required
        0, 0, 0, 0, 0, 0, // minimum price, every component: no floor
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, // maximum price, every component: no ceiling
        0, // max remaining hours: no limit
        static_cast<BYTE>(searchUtf8.size()),
        searchUtf8.empty() ? nullptr : reinterpret_cast<const BYTE*>(searchUtf8.data()),
        static_cast<uint32_t>(searchUtf8.size()));
}

void SEASON3B::CNewUIAuctionWindow::SubmitSearch()
{
    m_CurrentPage = 1;
    SendBrowseRequest();
    CUITextInputBox::ReleaseFocus();
    SetRelatedWnd(g_hWnd);
    PlayBuffer(SOUND_CLICK01);
}

void SEASON3B::CNewUIAuctionWindow::SetBrowseResponse(const AuctionHouse::AuctionBrowseResponse& response)
{
    if (!m_bBrowseRequestPending || response.RequestId != m_PendingBrowseRequestId)
    {
        return;
    }

    m_bBrowseRequestPending = false;
    if (m_iCurrentTab != TAB_BROWSE)
    {
        return;
    }

    m_bHasListingResponse = true;
    m_ListingResponse = response;
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
    m_BrowseScrollOffset = 0;
    const auto maximumOffset = AuctionHouse::MaximumBrowseScrollOffset(m_ListingResponse.Listings.size(), MaxBrowseRows);
    m_BrowseScrollBar.SetMaxPos(static_cast<int>(maximumOffset));
    m_BrowseScrollBar.SetCurPos(0);
    m_BrowseScrollBar.Show(maximumOffset > 0);
    RebuildRowItems();
}

void SEASON3B::CNewUIAuctionWindow::SendMyListingsRequest()
{
    m_bHasListingResponse = false;
    m_BrowseScrollOffset = 0;
    m_BrowseScrollBar.SetCurPos(0);
    m_BrowseScrollBar.Show(false);
    ReleaseRowItems();
    m_PendingMyListingsRequestId = AuctionHouse::NextAuctionRequestId();
    m_bMyListingsRequestPending = true;
    SocketClient->ToGameServer()->SendAuctionMyListingsRequest(
        m_PendingMyListingsRequestId,
        m_CurrentPage,
        AuctionHouse::EncodeListingStatusFilter(m_SelectedStatusIndex));
}

void SEASON3B::CNewUIAuctionWindow::SendMailboxRequest()
{
    m_bHasMailboxResponse = false;
    m_bHasSelectedCollection = false;
    m_BrowseScrollOffset = 0;
    m_BrowseScrollBar.SetCurPos(0);
    m_BrowseScrollBar.Show(false);
    ReleaseRowItems();
    m_PendingMailboxRequestId = AuctionHouse::NextAuctionRequestId();
    m_bMailboxRequestPending = true;
    SocketClient->ToGameServer()->SendAuctionMailboxRequest(
        m_PendingMailboxRequestId, m_CurrentPage,
        AuctionHouse::EncodeCollectionKindFilter(m_SelectedCollectionKindIndex));
}

void SEASON3B::CNewUIAuctionWindow::SendCurrentPageRequest()
{
    if (m_iCurrentTab == TAB_BROWSE)
    {
        SendBrowseRequest();
    }
    else if (m_iCurrentTab == TAB_MY_LISTINGS)
    {
        SendMyListingsRequest();
    }
    else if (m_iCurrentTab == TAB_MAILBOX)
    {
        SendMailboxRequest();
    }
}

void SEASON3B::CNewUIAuctionWindow::SetMyListingsResponse(const AuctionHouse::AuctionMyListingsResponse& response)
{
    if (!m_bMyListingsRequestPending || response.RequestId != m_PendingMyListingsRequestId)
    {
        return;
    }

    m_bMyListingsRequestPending = false;
    if (m_iCurrentTab != TAB_MY_LISTINGS)
    {
        return;
    }

    m_bHasListingResponse = true;
    m_ListingResponse = response;
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
    m_BrowseScrollOffset = 0;
    const auto maximumOffset = AuctionHouse::MaximumBrowseScrollOffset(m_ListingResponse.Listings.size(), MaxBrowseRows);
    m_BrowseScrollBar.SetMaxPos(static_cast<int>(maximumOffset));
    m_BrowseScrollBar.SetCurPos(0);
    m_BrowseScrollBar.Show(maximumOffset > 0);
    RebuildRowItems();
}

void SEASON3B::CNewUIAuctionWindow::SetMailboxResponse(const AuctionHouse::AuctionMailboxResponse& response)
{
    if (!m_bMailboxRequestPending || response.RequestId != m_PendingMailboxRequestId)
    {
        return;
    }

    m_bMailboxRequestPending = false;
    if (m_iCurrentTab != TAB_MAILBOX)
    {
        return;
    }

    m_bHasMailboxResponse = true;
    m_MailboxResponse = response;
    m_PendingMailboxCount = static_cast<uint16_t>(std::min<uint32_t>(response.TotalCount, UINT16_MAX));
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
    m_BrowseScrollOffset = 0;
    const auto maximumOffset = AuctionHouse::MaximumBrowseScrollOffset(m_MailboxResponse.Entries.size(), MaxBrowseRows);
    m_BrowseScrollBar.SetMaxPos(static_cast<int>(maximumOffset));
    m_BrowseScrollBar.SetCurPos(0);
    m_BrowseScrollBar.Show(maximumOffset > 0);
    RebuildRowItems();
}

void SEASON3B::CNewUIAuctionWindow::SetNotification(const AuctionHouse::AuctionNotification& notification)
{
    m_PendingMailboxCount = notification.PendingMailboxCount;
    const wchar_t* message = nullptr;
    switch (notification.NotificationKind)
    {
    case AuctionNotificationKind::Outbid: message = I18N::Game::AuctionNotificationOutbid; break;
    case AuctionNotificationKind::Sold: message = I18N::Game::AuctionNotificationSold; break;
    case AuctionNotificationKind::Bought: message = I18N::Game::AuctionNotificationBought; break;
    case AuctionNotificationKind::Expired: message = I18N::Game::AuctionNotificationExpired; break;
    case AuctionNotificationKind::Returned: message = I18N::Game::AuctionNotificationReturned; break;
    case AuctionNotificationKind::Collected: message = I18N::Game::AuctionNotificationCollected; break;
    case AuctionNotificationKind::AdminAdjusted: message = I18N::Game::AuctionNotificationAdminAdjusted; break;
    default: break;
    }
    if (message != nullptr && g_pSystemLogBox != nullptr)
    {
        g_pSystemLogBox->AddText(message, SEASON3B::TYPE_SYSTEM_MESSAGE);
    }

    if (m_iCurrentTab == TAB_MAILBOX && !m_bShowingDetail)
    {
        m_CurrentPage = 1;
        SendMailboxRequest();
    }
}

void SEASON3B::CNewUIAuctionWindow::SendDetailRequest(uint64_t listingId)
{
    m_bShowingDetail = true;
    m_SearchInput.SetState(UISTATE_HIDE);
    CUITextInputBox::ReleaseFocus();
    SetRelatedWnd(g_hWnd);
    m_bHasDetailResponse = false;
    m_PendingDetailRequestId = AuctionHouse::NextAuctionRequestId();
    m_bDetailRequestPending = true;
    m_bHasOperationResult = false;
    m_LastOperationMessage.clear();
    m_bOperationRequestPending = false;
    SocketClient->ToGameServer()->SendAuctionDetailRequest(m_PendingDetailRequestId, listingId);
}

void SEASON3B::CNewUIAuctionWindow::SetDetailResponse(const AuctionHouse::AuctionDetailResponse& response)
{
    if (!m_bDetailRequestPending || response.RequestId != m_PendingDetailRequestId)
    {
        return;
    }

    m_bDetailRequestPending = false;
    m_bHasDetailResponse = true;
    m_DetailResponse = response;
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
    RebuildDetailItem();
}

void SEASON3B::CNewUIAuctionWindow::SendBidRequest()
{
    // Defensive bounds re-check, same reasoning as the page buttons: the button's own Lock() state is only
    // refreshed once per Render() call and could in principle be one frame stale.
    if (!m_bHasDetailResponse || m_bOperationRequestPending)
    {
        return;
    }

    const auto minimumBid = AuctionHouse::ComputeMinimumNextBid(m_DetailResponse.CurrencyMode, m_DetailResponse.CurrentPrice, m_DetailResponse.BidCount);

    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_bOperationRequestPending = true;
    m_bHasOperationResult = false;

    const auto& fruits = minimumBid.Fruits();
    SocketClient->ToGameServer()->SendAuctionBidRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()),
        m_DetailResponse.ListingId, m_DetailResponse.Version,
        static_cast<uint32_t>(minimumBid.Scalar()),
        static_cast<uint32_t>(fruits.Strength), static_cast<uint32_t>(fruits.Agility),
        static_cast<uint32_t>(fruits.Vitality), static_cast<uint32_t>(fruits.Energy), static_cast<uint32_t>(fruits.Command));
}

void SEASON3B::CNewUIAuctionWindow::SendBuyoutRequest()
{
    if (!m_bHasDetailResponse || m_bOperationRequestPending || m_DetailResponse.BuyoutPrice.IsZero())
    {
        return;
    }

    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_bOperationRequestPending = true;
    m_bHasOperationResult = false;

    SocketClient->ToGameServer()->SendAuctionBuyoutRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()),
        m_DetailResponse.ListingId, m_DetailResponse.Version);
}

void SEASON3B::CNewUIAuctionWindow::RequestCancelConfirmation()
{
    if (m_iCurrentTab != TAB_MY_LISTINGS || !m_bShowingDetail || !m_bHasDetailResponse || m_bOperationRequestPending
        || !AuctionHouse::CanCancelOwnedListing(m_DetailResponse.Status, m_DetailResponse.BidCount))
    {
        return;
    }

    SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(AuctionCancelMsgBoxLayout));
}

void SEASON3B::CNewUIAuctionWindow::ConfirmCancelListing()
{
    if (m_iCurrentTab != TAB_MY_LISTINGS || !m_bShowingDetail || !m_bHasDetailResponse
        || m_bOperationRequestPending
        || !AuctionHouse::CanCancelOwnedListing(m_DetailResponse.Status, m_DetailResponse.BidCount))
    {
        return;
    }

    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_bOperationRequestPending = true;
    m_bHasOperationResult = false;
    SocketClient->ToGameServer()->SendAuctionCancelRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()),
        m_DetailResponse.ListingId, m_DetailResponse.Version);
}

void SEASON3B::CNewUIAuctionWindow::SetCollectionClaimInputsVisible(bool visible)
{
    const bool isFruit = visible && m_bHasSelectedCollection && !m_SelectedCollection.HasItem
        && m_SelectedCollection.CurrencyMode == AuctionCurrencyMode::Fruits;
    const bool isJewel = visible && m_bHasSelectedCollection && !m_SelectedCollection.HasItem
        && m_SelectedCollection.CurrencyMode > AuctionCurrencyMode::Zen
        && m_SelectedCollection.CurrencyMode < AuctionCurrencyMode::Fruits;
    for (size_t i = 0; i < std::size(m_CollectionClaimInputs); ++i)
    {
        const bool show = isFruit || (isJewel && i == 0);
        m_CollectionClaimInputs[i].SetState(show ? UISTATE_NORMAL : UISTATE_HIDE);
    }
    if (!visible && HasCollectionClaimInputFocus())
    {
        CUITextInputBox::ReleaseFocus();
        SetRelatedWnd(g_hWnd);
    }
}

void SEASON3B::CNewUIAuctionWindow::PopulateCollectionClaimInputs()
{
    SetCollectionClaimInputsVisible(false);
    if (!m_bHasSelectedCollection || m_SelectedCollection.HasItem
        || m_SelectedCollection.CurrencyMode == AuctionCurrencyMode::Zen)
    {
        return;
    }

    int64_t values[5]{};
    if (m_SelectedCollection.CurrencyMode == AuctionCurrencyMode::Fruits)
    {
        const auto& fruit = m_SelectedCollection.Remaining.Fruits();
        values[0] = fruit.Strength;
        values[1] = fruit.Agility;
        values[2] = fruit.Vitality;
        values[3] = fruit.Energy;
        values[4] = fruit.Command;
    }
    else
    {
        values[0] = m_SelectedCollection.Remaining.Scalar();
    }

    for (size_t i = 0; i < std::size(values); ++i)
    {
        wchar_t value[16];
        mu_swprintf(value, L"%lld", values[i]);
        m_CollectionClaimInputs[i].SetText(value);
    }
    SetCollectionClaimInputsVisible(true);
}

bool SEASON3B::CNewUIAuctionWindow::HasCollectionClaimInputFocus()
{
    return std::any_of(std::begin(m_CollectionClaimInputs), std::end(m_CollectionClaimInputs),
        [](CUITextInputBox& input) { return input.HaveFocus(); });
}

bool SEASON3B::CNewUIAuctionWindow::TryGetRequestedCollectionAmount(AuctionHouse::AuctionAmount& amount)
{
    // The wire's all-zero sentinel is the correct all-or-nothing request for lots and Zen.
    if (m_SelectedCollection.HasItem || m_SelectedCollection.CurrencyMode == AuctionCurrencyMode::Zen)
    {
        amount = AuctionHouse::AuctionAmount::FromScalar(0);
        return true;
    }

    int64_t values[5]{};
    const size_t valueCount = m_SelectedCollection.CurrencyMode == AuctionCurrencyMode::Fruits ? 5 : 1;
    for (size_t i = 0; i < valueCount; ++i)
    {
        wchar_t text[16]{};
        m_CollectionClaimInputs[i].GetText(text, static_cast<int>(std::size(text)));
        wchar_t* end = nullptr;
        const unsigned long long parsed = std::wcstoull(text, &end, 10);
        if (end == text || *end != L'\0' || parsed > UINT32_MAX)
        {
            return false;
        }
        values[i] = static_cast<int64_t>(parsed);
    }

    amount = m_SelectedCollection.CurrencyMode == AuctionCurrencyMode::Fruits
        ? AuctionHouse::AuctionAmount::FromFruits(AuctionHouse::FruitBasket{ values[0], values[1], values[2], values[3], values[4] })
        : AuctionHouse::AuctionAmount::FromScalar(values[0]);
    return AuctionHouse::IsValidCollectionClaim(m_SelectedCollection.CurrencyMode, m_SelectedCollection.Remaining, amount);
}

void SEASON3B::CNewUIAuctionWindow::SendCollectRequest()
{
    if (m_iCurrentTab != TAB_MAILBOX || !m_bShowingDetail || !m_bHasSelectedCollection
        || m_bOperationRequestPending || m_SelectedCollection.CollectionStatus == AuctionCollectionStatus::Claimed)
    {
        return;
    }

    AuctionHouse::AuctionAmount requested = AuctionHouse::AuctionAmount::FromScalar(0);
    if (!TryGetRequestedCollectionAmount(requested))
    {
        m_bHasOperationResult = true;
        m_LastOperationResult = AuctionResult::InvalidClaimQuantity;
        m_LastOperationMessage = I18N::Game::AuctionInvalidClaim;
        return;
    }

    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_bOperationRequestPending = true;
    m_bHasOperationResult = false;
    m_LastOperationMessage.clear();
    const auto& fruits = requested.Fruits();
    SocketClient->ToGameServer()->SendAuctionCollectRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()),
        m_SelectedCollection.CollectionId, m_SelectedCollection.Version,
        static_cast<uint32_t>(requested.Scalar()),
        static_cast<uint32_t>(fruits.Strength), static_cast<uint32_t>(fruits.Agility),
        static_cast<uint32_t>(fruits.Vitality), static_cast<uint32_t>(fruits.Energy),
        static_cast<uint32_t>(fruits.Command));
}

void SEASON3B::CNewUIAuctionWindow::SetOperationResponse(const AuctionHouse::AuctionOperationResponse& response)
{
    if (!m_bOperationRequestPending || response.OperationId != m_PendingOperationId)
    {
        return;
    }

    m_bOperationRequestPending = false;
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
    const AuctionResult result = response.Result;

    if (response.OperationType == AuctionOperationType::Collect)
    {
        m_bHasOperationResult = true;
        m_LastOperationResult = response.Result;
        switch (response.Result)
        {
        case AuctionResult::Success:
            if (m_bHasSelectedCollection && m_SelectedCollection.HasItem)
            {
                m_LastOperationMessage = I18N::Game::AuctionNotificationCollected;
            }
            else
            {
                const auto claimed = AuctionAmountText(response.CurrencyMode, response.Claimed);
                const auto remaining = AuctionAmountText(response.CurrencyMode, response.Remaining);
                wchar_t message[320];
                mu_swprintf(message, I18N::Game::AuctionCollectionResult, claimed.c_str(), remaining.c_str());
                m_LastOperationMessage = message;
            }
            break;
        case AuctionResult::InventoryFull:
            m_LastOperationMessage = I18N::Game::AuctionCollectionInventoryFull;
            break;
        case AuctionResult::MoneyLimit:
            m_LastOperationMessage = I18N::Game::AuctionCollectionMoneyLimit;
            break;
        case AuctionResult::StaleListing:
            m_LastOperationMessage = I18N::Game::AuctionCollectionStale;
            break;
        case AuctionResult::InvalidClaimQuantity:
            m_LastOperationMessage = I18N::Game::AuctionCollectionInvalidQuantity;
            break;
        default:
            m_LastOperationMessage = I18N::Game::Failed;
            break;
        }
        if (m_iCurrentTab == TAB_MAILBOX)
        {
            // Success removes or reduces the durable collection; a stale version or full inventory can also
            // change its authoritative state. In every case return to and refresh the server-owned page.
            m_bShowingDetail = false;
            m_bHasSelectedCollection = false;
            SetCollectionClaimInputsVisible(false);
            ReleaseDetailItem();
            SendMailboxRequest();
            m_bHasOperationResult = true;
            m_LastOperationResult = response.Result;
        }
        return;
    }

    if (response.OperationType == AuctionOperationType::Cancel)
    {
        if (response.Result == AuctionResult::Success && m_iCurrentTab == TAB_MY_LISTINGS)
        {
            // The item is now a durable Mailbox collection. Return to the owned-listing page and refresh it
            // from the server instead of mutating the cached row/status/version locally.
            m_bShowingDetail = false;
            ReleaseDetailItem();
            SendMyListingsRequest();
        }
        else if (response.Result != AuctionResult::Success && m_iCurrentTab == TAB_MY_LISTINGS
            && m_bShowingDetail && response.ListingId == m_DetailResponse.ListingId)
        {
            // StaleListing, CannotCancelWithBid and state changes all need a fresh authoritative snapshot.
            SendDetailRequest(response.ListingId);
        }

        m_bHasOperationResult = true;
        m_LastOperationResult = response.Result;
        return;
    }

    // The listing's price/version/bidder changed (win or lose), so refresh the detail panel from the server
    // rather than guessing the new state locally. Only while still looking at the same listing/tab — the
    // player may have already backed out or switched tabs by the time this reply arrives. SendDetailRequest
    // resets m_bHasOperationResult, so the result is (re-)applied after it, not before.
    if (m_bShowingDetail && m_iCurrentTab == TAB_BROWSE)
    {
        SendDetailRequest(response.ListingId);
    }

    m_bHasOperationResult = true;
    m_LastOperationResult = result;
}

void SEASON3B::CNewUIAuctionWindow::ReleaseRowItems()
{
    for (auto*& item : m_RowItems)
    {
        if (item != nullptr && g_pNewItemMng != nullptr)
        {
            g_pNewItemMng->DeleteItem(item);
        }
        item = nullptr;
    }
}

void SEASON3B::CNewUIAuctionWindow::RebuildRowItems()
{
    // Each response fully replaces the Browse page, so the previous page's owned items are always stale once a
    // new one arrives; release them all before creating this page's items, the same lifetime rule
    // CNewUIInventoryCtrl's own tooltip item follows (create fresh, delete the old one, never reuse).
    ReleaseRowItems();

    if (g_pNewItemMng == nullptr)
    {
        return;
    }

    if (m_iCurrentTab == TAB_MAILBOX)
    {
        const auto remainingRows = m_MailboxResponse.Entries.size() - std::min(m_BrowseScrollOffset, m_MailboxResponse.Entries.size());
        const auto rowCount = std::min(MaxBrowseRows, remainingRows);
        for (size_t row = 0; row < rowCount; ++row)
        {
            const auto& entry = m_MailboxResponse.Entries[m_BrowseScrollOffset + row];
            if (!entry.HasItem || entry.ItemDataLength == 0)
            {
                continue;
            }
            const size_t length = std::min<size_t>(entry.ItemDataLength, entry.ItemData.size());
            m_RowItems[row] = g_pNewItemMng->CreateItem(std::span<const BYTE>(entry.ItemData.data(), length));
        }
        return;
    }

    const auto remainingRows = m_ListingResponse.Listings.size() - std::min(m_BrowseScrollOffset, m_ListingResponse.Listings.size());
    const auto rowCount = std::min(MaxBrowseRows, remainingRows);
    for (size_t row = 0; row < rowCount; ++row)
    {
        const auto& listing = m_ListingResponse.Listings[m_BrowseScrollOffset + row];
        if (listing.ItemDataLength == 0)
        {
            continue;
        }

        const size_t length = std::min<size_t>(listing.ItemDataLength, listing.ItemData.size());
        m_RowItems[row] = g_pNewItemMng->CreateItem(std::span<const BYTE>(listing.ItemData.data(), length));
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderRowItemTooltip(int row) const
{
    if (row < 0 || static_cast<size_t>(row) >= MaxBrowseRows || m_RowItems[row] == nullptr)
    {
        return;
    }

    const int iconX = m_Pos.x + BROWSE_BODY_X
        + (m_iCurrentTab == TAB_MAILBOX ? MAILBOX_CONTENT_COLUMN_X : 0);
    const int iconY = m_Pos.y + BROWSE_BODY_Y + row * BROWSE_ROW_HEIGHT + BROWSE_ICON_MARGIN;
    RenderItemInfo(iconX + BROWSE_ICON_SIZE / 2, iconY + BROWSE_ICON_SIZE / 2, m_RowItems[row], false);
}

void SEASON3B::CNewUIAuctionWindow::ReleaseDetailItem()
{
    if (m_DetailItem != nullptr && g_pNewItemMng != nullptr)
    {
        g_pNewItemMng->DeleteItem(m_DetailItem);
    }
    m_DetailItem = nullptr;
}

void SEASON3B::CNewUIAuctionWindow::RebuildDetailItem()
{
    // Same create-fresh/delete-old lifetime as RebuildRowItems, but kept as its own owned pointer rather than
    // borrowing a row's item: the detail response's ItemData is its own independent snapshot, and the row that
    // was clicked may no longer exist by the time this response arrives (the Browse page can be refreshed, or
    // paged away from, while a detail request is in flight).
    ReleaseDetailItem();

    if (g_pNewItemMng == nullptr)
    {
        return;
    }

    if (m_iCurrentTab == TAB_MAILBOX)
    {
        if (!m_bHasSelectedCollection || !m_SelectedCollection.HasItem || m_SelectedCollection.ItemDataLength == 0)
        {
            return;
        }
        const size_t length = std::min<size_t>(m_SelectedCollection.ItemDataLength, m_SelectedCollection.ItemData.size());
        m_DetailItem = g_pNewItemMng->CreateItem(std::span<const BYTE>(m_SelectedCollection.ItemData.data(), length));
        return;
    }

    if (m_DetailResponse.ItemDataLength == 0)
    {
        return;
    }

    const size_t length = std::min<size_t>(m_DetailResponse.ItemDataLength, m_DetailResponse.ItemData.size());
    m_DetailItem = g_pNewItemMng->CreateItem(std::span<const BYTE>(m_DetailResponse.ItemData.data(), length));
}

void SEASON3B::CNewUIAuctionWindow::RenderDetailItemTooltip() const
{
    if (m_DetailItem == nullptr)
    {
        return;
    }

    const int iconX = m_Pos.x + BROWSE_BODY_X;
    const int iconY = m_Pos.y + BROWSE_BODY_Y;
    RenderItemInfo(iconX + DETAIL_ICON_SIZE / 2, iconY + DETAIL_ICON_SIZE / 2, m_DetailItem, false);
}

void SEASON3B::CNewUIAuctionWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD /*dwParamB*/)
{
    if (pClass == nullptr)
    {
        return;
    }

    // Real row indexes are below MaxBrowseRows. The two following values identify the detail and Sell items;
    // one callback function pointer serves every case because CNewUI3DRenderMng::DeleteUI2DEffectObject
    // matches by function pointer, not by instance.
    auto* window = static_cast<CNewUIAuctionWindow*>(pClass);
    if (static_cast<size_t>(dwParamA) == MaxBrowseRows)
    {
        window->RenderDetailItemTooltip();
    }
    else if (static_cast<size_t>(dwParamA) == MaxBrowseRows + 1)
    {
        window->RenderSellItemTooltip();
    }
    else
    {
        window->RenderRowItemTooltip(static_cast<int>(dwParamA));
    }
}

void SEASON3B::CNewUIAuctionWindow::SetOpenResponse(const AuctionHouse::AuctionOpenResponse& response)
{
    if (!m_bOpenRequestPending || response.RequestId != m_PendingOpenRequestId)
    {
        return;
    }

    m_bOpenRequestPending = false;
    m_bHasOpenResponse = true;
    m_OpenResponse = response;
    m_PendingMailboxCount = response.PendingMailboxCount;
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
}

void SEASON3B::CNewUIAuctionWindow::ClosingProcess()
{
    LeaveSellTab();
    m_bDragging = false;
    m_SearchInput.SetState(UISTATE_HIDE);
    SetCollectionClaimInputsVisible(false);
    if (m_SearchInput.HaveFocus())
    {
        CUITextInputBox::ReleaseFocus();
    }
    SetRelatedWnd(g_hWnd);
}

void SEASON3B::CNewUIAuctionWindow::EnterSellTab()
{
    // Browse needs the owner's 600-wide table, but that cannot coexist with the stock 190-wide inventory on
    // the 640-unit panel canvas. Sell returns to the approved original 400-wide geometry and uses the same
    // right-docked transform as inventory, putting the two windows side by side at every supported aspect.
    SetLayoutMode(UI::Scaling::LayoutMode::DockRight);
    RepositionChildren();
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
    {
        m_bOpenedInventoryForSell = true;
        g_pMyInventory->SetPos(m_Pos.x + CurrentWindowWidth() + 10, m_Pos.y);
        g_pNewUISystem->Show(SEASON3B::INTERFACE_INVENTORY);
    }
}

void SEASON3B::CNewUIAuctionWindow::LeaveSellTab()
{
    if (m_bOpenedInventoryForSell && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY);
    }

    m_bOpenedInventoryForSell = false;
    m_bPointingSellItem = false;
}

void SEASON3B::CNewUIAuctionWindow::CaptureSellSelection()
{
    auto* picked = CNewUIInventoryCtrl::GetPickedItem();
    if (picked == nullptr || picked->GetSourceStorageType() != STORAGE_TYPE::INVENTORY)
    {
        return;
    }

    const int slot = picked->GetSourceLinealPos();
    ITEM* item = picked->GetItem();
    if (item != nullptr && IsPlayerInventorySlot(slot))
    {
        m_SellInventorySlot = static_cast<BYTE>(slot);
        m_SellItemSnapshot = *item;
        m_bHasSellItem = true;
    }

    // Selection is not an item move. Put the engine-owned picked item back in its source slot immediately.
    // If the legacy restore cannot reinsert it (for example because its client slot map is already stale),
    // request the authoritative inventory and release the cursor copy so Sell never leaves the giant red
    // invalid-drop overlay seen in the first real-game pass.
    CNewUIInventoryCtrl::BackupPickedItem();
    if (CNewUIInventoryCtrl::GetPickedItem() == picked)
    {
        SocketClient->ToGameServer()->SendInventoryRequest();
        CNewUIInventoryCtrl::DeletePickedItem();
    }

    // Inventory pickup is release-triggered. Consume that release after restoring the item, otherwise
    // the inventory control can see it again on the next update and immediately pick the same item back up.
    g_pMyInventory->ResetMouseLButton();
    g_pNewKeyInput->SetKeyState(VK_LBUTTON, CNewKeyInput::KEY_NONE);
}

void SEASON3B::CNewUIAuctionWindow::ValidateSellSelection()
{
    if (!m_bHasSellItem)
    {
        return;
    }

    const ITEM* liveItem = FindInventoryItemBySlot(m_SellInventorySlot);
    if (liveItem == nullptr || liveItem->Key != m_SellItemSnapshot.Key)
    {
        m_bHasSellItem = false;
        m_bPointingSellItem = false;
    }
}

bool SEASON3B::CNewUIAuctionWindow::UpdateMouseEvent()
{
    if (BtnProcess())
    {
        return false;
    }

    if (m_bDragging)
    {
        // MouseLButton (not a one-shot press-edge flag) is required here: using an edge-triggered press to
        // decide whether to KEEP dragging ends the drag one frame after every press. Same reasoning as
        // CNewUIExpHuntWindow's own drag loop.
        if (!MouseLButton)
        {
            m_bDragging = false;
            return false;
        }

        m_Pos.x = MouseX - m_iDragGrabOffsetX;
        m_Pos.y = MouseY - m_iDragGrabOffsetY;
        RepositionChildren();
        return false;
    }

    // Grab zone is the title strip above the tabs, excluding the close button's own rect (BtnProcess already
    // handles a close-button click above, before this is ever reached, but excluding it here too keeps the
    // grab zone honest if that ordering ever changes).
    const int windowWidth = CurrentWindowWidth();
    const bool overTitleBar = CheckMouseIn(m_Pos.x, m_Pos.y, windowWidth - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN, TAB_REGION_Y);
    if (overTitleBar && IsPress(VK_LBUTTON))
    {
        m_bDragging = true;
        m_iDragGrabOffsetX = MouseX - m_Pos.x;
        m_iDragGrabOffsetY = MouseY - m_Pos.y;
        return false;
    }

    if (CheckMouseIn(m_Pos.x, m_Pos.y, windowWidth, WINDOW_HEIGHT))
    {
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIAuctionWindow::BtnProcess()
{
    if (m_BtnClose.UpdateMouseEvent() == true)
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_AUCTION_HOUSE);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS || m_iCurrentTab == TAB_MAILBOX) && m_bShowingDetail)
    {
        if (m_BtnBack.UpdateMouseEvent() == true)
        {
            m_bShowingDetail = false;
            m_bHasSelectedCollection = false;
            SetCollectionClaimInputsVisible(false);
            m_SearchInput.SetState(m_iCurrentTab == TAB_BROWSE ? UISTATE_NORMAL : UISTATE_HIDE);
            ReleaseDetailItem();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_iCurrentTab == TAB_MAILBOX)
        {
            for (auto& input : m_CollectionClaimInputs)
            {
                input.DoAction();
                if (input.HaveFocus())
                {
                    SetRelatedWnd(input.GetHandle());
                }
                if (input.GetState() == UISTATE_NORMAL
                    && CheckMouseIn(input.GetPosition_x(), input.GetPosition_y(), CLAIM_INPUT_WIDTH, CLAIM_INPUT_HEIGHT))
                {
                    return true;
                }
            }
        }

        if (m_iCurrentTab == TAB_BROWSE && m_BtnBid.UpdateMouseEvent() == true)
        {
            SendBidRequest();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_iCurrentTab == TAB_BROWSE && m_BtnBuyout.UpdateMouseEvent() == true)
        {
            SendBuyoutRequest();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_iCurrentTab == TAB_MY_LISTINGS && m_BtnCancelListing.UpdateMouseEvent() == true)
        {
            RequestCancelConfirmation();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_iCurrentTab == TAB_MAILBOX && m_BtnCollect.UpdateMouseEvent() == true)
        {
            SendCollectRequest();
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        return false;
    }

    if (m_SearchInput.HaveFocus() && IsPress(VK_LBUTTON)
        && !CheckMouseIn(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET,
            SEARCH_INPUT_WIDTH, SEARCH_INPUT_HEIGHT))
    {
        CUITextInputBox::ReleaseFocus();
        SetRelatedWnd(g_hWnd);
    }

    // The combo's own contract: its expanded dropdown can extend past this widget's own small hit box, so
    // the owner must separately treat IsMouseOverWidget() as a consumed click.
    if (m_iCurrentTab == TAB_BROWSE && m_CurrencyCombo.IsMouseOverWidget())
    {
        return true;
    }

    if (m_iCurrentTab == TAB_MY_LISTINGS && m_StatusCombo.IsMouseOverWidget())
    {
        return true;
    }

    if (m_iCurrentTab == TAB_MAILBOX && m_CollectionKindCombo.IsMouseOverWidget())
    {
        return true;
    }

    if (m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS || m_iCurrentTab == TAB_MAILBOX)
    {
        if (m_iCurrentTab == TAB_BROWSE)
        {
            if (IsRelease(VK_LBUTTON)
                && CheckMouseIn(m_Pos.x + CATEGORY_RAIL_X, m_Pos.y + CATEGORY_RAIL_Y,
                    CATEGORY_RAIL_WIDTH, CATEGORY_RAIL_ROW_HEIGHT * static_cast<int>(std::size(m_CategoryLabels))))
            {
                const int selectedCategory = (MouseY - (m_Pos.y + CATEGORY_RAIL_Y)) / CATEGORY_RAIL_ROW_HEIGHT;
                if (selectedCategory != m_SelectedCategoryIndex)
                {
                    m_SelectedCategoryIndex = selectedCategory;
                    m_CategoryCombo.SetSelectedIndex(selectedCategory);
                    m_CurrentPage = 1;
                    SendBrowseRequest();
                }
                PlayBuffer(SOUND_CLICK01);
                return true;
            }

            m_SearchInput.DoAction();
            if (m_SearchInput.HaveFocus())
            {
                SetRelatedWnd(m_SearchInput.GetHandle());
            }
            if (CheckMouseIn(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET,
                SEARCH_INPUT_WIDTH, SEARCH_INPUT_HEIGHT))
            {
                return true;
            }

            if (m_BtnSearch.UpdateMouseEvent() == true)
            {
                SubmitSearch();
                return true;
            }
        }

        if (m_BrowseScrollBar.IsVisible())
        {
            m_BrowseScrollBar.UpdateMouseEvent();
        }

        if (m_BrowseScrollBar.IsVisible()
            && CheckMouseIn(m_Pos.x + BROWSE_TABLE_X, m_Pos.y + BROWSE_BODY_Y,
            BROWSE_TABLE_CONTENT_WIDTH, BROWSE_SCROLLBAR_HEIGHT) && MouseWheel != 0)
        {
            m_BrowseScrollBar.SetCurPos(m_BrowseScrollBar.GetCurPos() - MouseWheel);
            MouseWheel = 0;
            const auto newOffset = static_cast<size_t>(m_BrowseScrollBar.GetCurPos());
            if (newOffset != m_BrowseScrollOffset)
            {
                m_BrowseScrollOffset = newOffset;
                RebuildRowItems();
            }
            return true;
        }

        if (m_iCurrentTab == TAB_BROWSE && IsRelease(VK_LBUTTON)
            && CheckMouseIn(m_Pos.x + BROWSE_TABLE_X, m_Pos.y + BROWSE_HEADER_Y,
                BROWSE_TABLE_CONTENT_WIDTH, BROWSE_HEADER_HEIGHT))
        {
            const int localX = MouseX - (m_Pos.x + BROWSE_TABLE_X);
            AuctionSort requestedSort = m_SelectedSort;
            if (localX >= BROWSE_TIME_COLUMN_X && localX < BROWSE_TIME_COLUMN_X + BROWSE_TIME_COLUMN_WIDTH)
            {
                requestedSort = AuctionHouse::NextBrowseSort(AuctionHouse::AuctionBrowseSortColumn::TimeLeft, m_SelectedSort);
            }
            else if (m_SelectedCurrency != AuctionCurrencyMode::Fruits
                && localX >= BROWSE_PRICE_COLUMN_X && localX < BROWSE_PRICE_COLUMN_X + BROWSE_PRICE_COLUMN_WIDTH)
            {
                requestedSort = AuctionHouse::NextBrowseSort(AuctionHouse::AuctionBrowseSortColumn::Price, m_SelectedSort);
            }

            if (requestedSort != m_SelectedSort)
            {
                m_SelectedSort = requestedSort;
                m_CurrentPage = 1;
                SendBrowseRequest();
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }

        if (m_BtnPrevPage.UpdateMouseEvent() == true)
        {
            if (m_CurrentPage > 1)
            {
                --m_CurrentPage;
                SendCurrentPageRequest();
            }
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_BtnNextPage.UpdateMouseEvent() == true)
        {
            const uint16_t totalPages = m_iCurrentTab == TAB_MAILBOX
                ? (m_bHasMailboxResponse ? m_MailboxResponse.TotalPages : 0)
                : (m_bHasListingResponse ? m_ListingResponse.TotalPages : 0);
            if (m_CurrentPage < totalPages)
            {
                ++m_CurrentPage;
                SendCurrentPageRequest();
            }
            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_iPointedRow != -1 && IsRelease(VK_LBUTTON))
        {
            if (m_iCurrentTab == TAB_MAILBOX)
            {
                m_SelectedCollection = m_MailboxResponse.Entries[m_BrowseScrollOffset + m_iPointedRow];
                m_bHasSelectedCollection = true;
                m_bShowingDetail = true;
                m_bHasOperationResult = false;
                m_LastOperationMessage.clear();
                RebuildDetailItem();
                PopulateCollectionClaimInputs();
            }
            else if (!m_bDetailRequestPending)
            {
                SendDetailRequest(m_ListingResponse.Listings[m_BrowseScrollOffset + m_iPointedRow].ListingId);
            }
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }

    return false;
}

bool SEASON3B::CNewUIAuctionWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_AUCTION_HOUSE) == true)
    {
        if (HasCollectionClaimInputFocus())
        {
            if (SEASON3B::IsPress(VK_RETURN) == true)
            {
                SendCollectRequest();
                CUITextInputBox::ReleaseFocus();
                SetRelatedWnd(g_hWnd);
                return false;
            }
            if (SEASON3B::IsPress(VK_ESCAPE) == true)
            {
                CUITextInputBox::ReleaseFocus();
                SetRelatedWnd(g_hWnd);
                return false;
            }
            return true;
        }

        if (m_SearchInput.HaveFocus())
        {
            if (SEASON3B::IsPress(VK_RETURN) == true)
            {
                SubmitSearch();
                return false;
            }

            if (SEASON3B::IsPress(VK_ESCAPE) == true)
            {
                CUITextInputBox::ReleaseFocus();
                SetRelatedWnd(g_hWnd);
                return false;
            }

            return true;
        }

        if (SEASON3B::IsPress(VK_ESCAPE) == true)
        {
            g_pNewUISystem->Hide(SEASON3B::INTERFACE_AUCTION_HOUSE);
            PlayBuffer(SOUND_CLICK01);

            return false;
        }
    }

    return true;
}

bool SEASON3B::CNewUIAuctionWindow::Update()
{
    if (IsVisible())
    {
        const int selected = m_TabBtn.UpdateMouseEvent();
        if (selected != RADIOGROUPEVENT_NONE && selected != m_iCurrentTab)
        {
            if (m_iCurrentTab == TAB_SELL)
            {
                LeaveSellTab();
            }

            m_iCurrentTab = selected;
            if (m_iCurrentTab == TAB_SELL)
            {
                EnterSellTab();
            }
            else
            {
                SetLayoutMode(UI::Scaling::LayoutMode::Dialog);
                RepositionChildren();
            }

            m_bShowingDetail = false;
            m_bHasSelectedCollection = false;
            SetCollectionClaimInputsVisible(false);
            m_bHasOperationResult = false;
            m_LastOperationMessage.clear();
            ReleaseDetailItem();
            m_SearchInput.SetState(m_iCurrentTab == TAB_BROWSE ? UISTATE_NORMAL : UISTATE_HIDE);
            if (m_iCurrentTab != TAB_BROWSE && m_SearchInput.HaveFocus())
            {
                CUITextInputBox::ReleaseFocus();
                SetRelatedWnd(g_hWnd);
            }

            m_CurrentPage = 1;
            m_BrowseScrollOffset = 0;
            if (m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS || m_iCurrentTab == TAB_MAILBOX)
            {
                SendCurrentPageRequest();
            }
            else
            {
                m_bHasListingResponse = false;
                m_BrowseScrollBar.Show(false);
                ReleaseRowItems();
            }
        }

        if (m_iCurrentTab == TAB_BROWSE && !m_bShowingDetail && m_CurrencyCombo.UpdateMouseEvent())
        {
            m_SelectedCurrency = static_cast<AuctionCurrencyMode>(m_CurrencyCombo.GetSelectedIndex());
            m_SelectedSort = AuctionHouse::NormalizeBrowseSort(m_SelectedCurrency, m_SelectedSort);
            m_CurrentPage = 1;
            SendBrowseRequest();
        }

        if (m_iCurrentTab == TAB_BROWSE && !m_bShowingDetail && m_CategoryCombo.UpdateMouseEvent())
        {
            m_SelectedCategoryIndex = m_CategoryCombo.GetSelectedIndex();
            m_CurrentPage = 1;
            SendBrowseRequest();
        }

        if (m_iCurrentTab == TAB_MY_LISTINGS && !m_bShowingDetail && m_StatusCombo.UpdateMouseEvent())
        {
            m_SelectedStatusIndex = m_StatusCombo.GetSelectedIndex();
            m_CurrentPage = 1;
            SendMyListingsRequest();
        }


        if (m_iCurrentTab == TAB_MAILBOX && !m_bShowingDetail && m_CollectionKindCombo.UpdateMouseEvent())
        {
            m_SelectedCollectionKindIndex = m_CollectionKindCombo.GetSelectedIndex();
            m_CurrentPage = 1;
            SendMailboxRequest();
        }

        if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS || m_iCurrentTab == TAB_MAILBOX)
            && !m_bShowingDetail
            && (m_iCurrentTab == TAB_MAILBOX ? m_bHasMailboxResponse : m_bHasListingResponse))
        {
            m_BrowseScrollBar.Update();
            const auto newOffset = static_cast<size_t>(m_BrowseScrollBar.GetCurPos());
            if (newOffset != m_BrowseScrollOffset)
            {
                m_BrowseScrollOffset = newOffset;
                RebuildRowItems();
            }
        }

        m_iPointedRow = -1;
        m_bPointingDetailItem = false;
        m_bPointingSellItem = false;
        if (m_iCurrentTab == TAB_SELL)
        {
            CaptureSellSelection();
            ValidateSellSelection();
            const int iconX = m_Pos.x + SELL_PANEL_X + SELL_ICON_MARGIN;
            const int iconY = m_Pos.y + SELL_PANEL_Y + (SELL_PANEL_HEIGHT - SELL_ICON_SIZE) / 2;
            m_bPointingSellItem = m_bHasSellItem && CheckMouseIn(iconX, iconY, SELL_ICON_SIZE, SELL_ICON_SIZE);
        }
        if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS || m_iCurrentTab == TAB_MAILBOX) && m_bShowingDetail)
        {
            m_bPointingDetailItem = CheckMouseIn(m_Pos.x + BROWSE_BODY_X, m_Pos.y + BROWSE_BODY_Y, DETAIL_ICON_SIZE, DETAIL_ICON_SIZE);
        }
        else if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS) && m_bHasListingResponse)
        {
            const auto remainingRows = m_ListingResponse.Listings.size() - std::min(m_BrowseScrollOffset, m_ListingResponse.Listings.size());
            const auto rowCount = std::min(MaxBrowseRows, remainingRows);
            for (size_t row = 0; row < rowCount; ++row)
            {
                if (CheckMouseIn(m_Pos.x + BROWSE_TABLE_X, m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT,
                    BROWSE_TABLE_CONTENT_WIDTH, BROWSE_ROW_HEIGHT))
                {
                    m_iPointedRow = static_cast<int>(row);
                    break;
                }
            }
        }
        else if (m_iCurrentTab == TAB_MAILBOX && m_bHasMailboxResponse)
        {
            const auto remainingRows = m_MailboxResponse.Entries.size() - std::min(m_BrowseScrollOffset, m_MailboxResponse.Entries.size());
            const auto rowCount = std::min(MaxBrowseRows, remainingRows);
            for (size_t row = 0; row < rowCount; ++row)
            {
                if (CheckMouseIn(m_Pos.x + BROWSE_BODY_X, m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT,
                    BROWSE_CONTENT_WIDTH, BROWSE_ROW_HEIGHT))
                {
                    m_iPointedRow = static_cast<int>(row);
                    break;
                }
            }
        }
    }

    return true;
}

bool SEASON3B::CNewUIAuctionWindow::Render()
{
    EnableAlphaTest();

    RenderFrame();

    m_TabBtn.Render();
    m_BtnClose.Render();

    if (m_PendingMailboxCount > 0)
    {
        const int tabWidth = (CurrentWindowWidth() - 2 * TAB_REGION_X) / 4;
        wchar_t badge[16];
        mu_swprintf(badge, L"[%d]", m_PendingMailboxCount);
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(255, 190, 70, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(
            (float)(m_Pos.x + TAB_REGION_X + 4 * tabWidth - 38),
            (float)(m_Pos.y + TAB_REGION_Y + 8), badge, 34.0f, 0, RT3_SORT_CENTER);
    }

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText((float)(m_Pos.x + 15), (float)(m_Pos.y + 13), I18N::Game::AuctionHouse,
        (float)(CurrentWindowWidth() - 30), 0, RT3_SORT_CENTER);

    if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS) && m_bShowingDetail)
    {
        RenderDetailPanel();
        m_BtnBack.Render();
        RenderOperationButtons();
    }
    else if (m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS)
    {
        if (m_iCurrentTab == TAB_BROWSE)
        {
            RenderCategoryRail();
        }
        RenderBrowseHeader();
        RenderBrowseTab();
        if (m_iCurrentTab == TAB_BROWSE)
        {
            RenderCurrencyBalances();
        }
        RenderPageControls();
        if (m_BrowseScrollBar.IsVisible())
        {
            m_BrowseScrollBar.Render();
        }

        if (m_iCurrentTab == TAB_BROWSE)
        {
            m_SearchInput.Render();
            m_BtnSearch.Render();
            m_CurrencyCombo.Render();
        }
        else
        {
            m_StatusCombo.Render();
        }
    }
    else if (m_iCurrentTab == TAB_MAILBOX && m_bShowingDetail)
    {
        RenderMailboxDetailPanel();
        m_BtnBack.Render();
        m_BtnCollect.Render();
    }
    else if (m_iCurrentTab == TAB_MAILBOX)
    {
        RenderMailboxHeader();
        RenderMailboxTab();
        RenderPageControls();
        if (m_BrowseScrollBar.IsVisible())
        {
            m_BrowseScrollBar.Render();
        }
        m_CollectionKindCombo.Render();
    }
    else if (m_iCurrentTab == TAB_SELL)
    {
        RenderSellTab();
    }

    DisableAlphaBlend();

    return true;
}

void SEASON3B::CNewUIAuctionWindow::RenderDetailPanel()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    constexpr int BodyWidth = WINDOW_WIDTH - 2 * BROWSE_BODY_X;

    if (!m_bHasDetailResponse)
    {
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)(m_Pos.y + BROWSE_BODY_Y), I18N::Game::PleaseWait, (float)BodyWidth, 0, RT3_SORT_LEFT);
        return;
    }

    const auto& detail = m_DetailResponse;
    const uint32_t estimatedServerTime = m_ServerClock.EstimatedServerTime(GetTickCount());
    const auto remainingSeconds = detail.EndsAt > estimatedServerTime ? (detail.EndsAt - estimatedServerTime) : 0;
    const auto countdown = AuctionHouse::FormatAuctionCountdown(std::chrono::seconds(remainingSeconds));
    const std::wstring priceText = detail.CurrentPrice.IsFruitBasket() ? L"(fruits)" : std::to_wstring(detail.CurrentPrice.Scalar());
    const std::wstring buyoutText = detail.BuyoutPrice.IsZero()
        ? L"\x2014" // em dash: no buyout set, per the design spec's own "buyout price, or an em dash" wording.
        : (detail.BuyoutPrice.IsFruitBasket() ? L"(fruits)" : std::to_wstring(detail.BuyoutPrice.Scalar()));

    const int textX = m_Pos.x + BROWSE_BODY_X + DETAIL_TEXT_X_OFFSET;
    int textY = m_Pos.y + BROWSE_BODY_Y;
    const int textWidth = BodyWidth - DETAIL_TEXT_X_OFFSET;

    if (m_DetailItem != nullptr)
    {
        const std::wstring itemName = GetItemDisplayName(m_DetailItem);
        g_pRenderText->RenderText((float)textX, (float)textY, itemName.c_str(), (float)textWidth, 0, RT3_SORT_LEFT);
    }
    textY += DETAIL_ICON_SIZE + 6;

    wchar_t line[256];
    mu_swprintf(line, L"%ls: %ls", I18N::Game::Seller, detail.SellerName.c_str());
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)textY, line, (float)BodyWidth, 0, RT3_SORT_LEFT);
    textY += DETAIL_LINE_HEIGHT;

    mu_swprintf(line, L"%ls / %ls   x%d   %ls", priceText.c_str(), buyoutText.c_str(), detail.BidCount, countdown.c_str());
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)textY, line, (float)BodyWidth, 0, RT3_SORT_LEFT);
    textY += DETAIL_LINE_HEIGHT;

    if (!detail.CurrentBidderName.empty())
    {
        mu_swprintf(line, L"%ls: %ls", I18N::Game::Bidder, detail.CurrentBidderName.c_str());
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)textY, line, (float)BodyWidth, 0, RT3_SORT_LEFT);
        textY += DETAIL_LINE_HEIGHT;
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderMailboxDetailPanel()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    constexpr int BodyWidth = WINDOW_WIDTH - 2 * BROWSE_BODY_X;

    if (!m_bHasSelectedCollection)
    {
        return;
    }

    const auto& entry = m_SelectedCollection;
    const int textX = m_Pos.x + BROWSE_BODY_X + (entry.HasItem ? DETAIL_TEXT_X_OFFSET : 0);
    int textY = m_Pos.y + BROWSE_BODY_Y;
    const int textWidth = BodyWidth - (entry.HasItem ? DETAIL_TEXT_X_OFFSET : 0);
    g_pRenderText->RenderText((float)textX, (float)textY, CollectionKindText(entry.CollectionKind),
        (float)textWidth, 0, RT3_SORT_LEFT);
    textY += DETAIL_ICON_SIZE + 6;

    const auto amount = entry.HasItem ? std::wstring(I18N::Game::Item)
        : AuctionAmountText(entry.CurrencyMode, entry.Remaining);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)textY, amount.c_str(),
        (float)BodyWidth, 0, RT3_SORT_LEFT);
    textY += DETAIL_LINE_HEIGHT;

    wchar_t line[160];
    mu_swprintf(line, L"%ls: %llu", I18N::Game::AuctionSource, entry.SourceListingId);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)textY, line,
        (float)BodyWidth, 0, RT3_SORT_LEFT);
    textY += DETAIL_LINE_HEIGHT;
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)textY,
        CollectionStatusText(entry.CollectionStatus), (float)BodyWidth, 0, RT3_SORT_LEFT);

    const wchar_t* fruitLabels[] = {
        I18N::Game::Strength, I18N::Game::Agility, I18N::Game::Vitality, I18N::Game::Energy, I18N::Game::Command
    };
    if (!entry.HasItem && entry.CurrencyMode != AuctionCurrencyMode::Zen)
    {
        const size_t inputCount = entry.CurrencyMode == AuctionCurrencyMode::Fruits ? 5 : 1;
        for (size_t i = 0; i < inputCount; ++i)
        {
            const int x = m_Pos.x + TOOLBAR_X + static_cast<int>(i) * CLAIM_INPUT_COLUMN_WIDTH;
            const wchar_t* label = inputCount == 1 ? I18N::Game::AuctionClaimAmount : fruitLabels[i];
            g_pRenderText->RenderText((float)x, (float)(m_Pos.y + CLAIM_INPUT_Y_OFFSET), label,
                (float)CLAIM_INPUT_COLUMN_WIDTH, 0, RT3_SORT_LEFT);
            m_CollectionClaimInputs[i].Render();
        }
    }

    AuctionHouse::AuctionAmount requested = AuctionHouse::AuctionAmount::FromScalar(0);
    const bool validClaim = TryGetRequestedCollectionAmount(requested);
    const bool canCollect = !m_bOperationRequestPending && validClaim
        && entry.CollectionStatus != AuctionCollectionStatus::Claimed;
    if (canCollect)
    {
        m_BtnCollect.UnLock();
        m_BtnCollect.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
        m_BtnCollect.ChangeTextColor(RGBA(255, 255, 255, 255));
    }
    else
    {
        m_BtnCollect.Lock();
        m_BtnCollect.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
        m_BtnCollect.ChangeTextColor(RGBA(100, 100, 100, 255));
    }

    if (!validClaim)
    {
        g_pRenderText->SetTextColor(255, 90, 90, 255);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X),
            (float)(m_Pos.y + CLAIM_INPUT_Y_OFFSET + CLAIM_INPUT_HEIGHT + 20),
            I18N::Game::AuctionInvalidClaim, (float)BodyWidth, 0, RT3_SORT_LEFT);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderOperationButtons()
{
    if (m_iCurrentTab == TAB_MY_LISTINGS)
    {
        const bool canCancel = m_bHasDetailResponse && !m_bOperationRequestPending
            && AuctionHouse::CanCancelOwnedListing(m_DetailResponse.Status, m_DetailResponse.BidCount);
        if (canCancel)
        {
            m_BtnCancelListing.UnLock();
            m_BtnCancelListing.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
            m_BtnCancelListing.ChangeTextColor(RGBA(255, 255, 255, 255));
        }
        else
        {
            m_BtnCancelListing.Lock();
            m_BtnCancelListing.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
            m_BtnCancelListing.ChangeTextColor(RGBA(100, 100, 100, 255));
        }

        m_BtnCancelListing.Render();

        if (m_bHasOperationResult)
        {
            const bool succeeded = m_LastOperationResult == AuctionResult::Success;
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(succeeded ? 120 : 255, succeeded ? 220 : 90, 120, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText((float)m_Pos.x, (float)(m_Pos.y + OPERATION_RESULT_Y_OFFSET),
                succeeded ? I18N::Game::Success : I18N::Game::Failed, (float)WINDOW_WIDTH, 0, RT3_SORT_CENTER);
        }
        return;
    }

    // Locked (and grayed) while no detail response has arrived yet, while a mutation is already in flight, or
    // (Buyout only) when the listing has no buyout price. The server independently re-validates every other
    // rule (self-trade, stale version, already-highest-bidder, etc.) — this is guidance, not enforcement, the
    // same relationship the page buttons have with their own boundary check.
    const bool canBid = m_bHasDetailResponse && !m_bOperationRequestPending;
    const bool canBuyout = canBid && !m_DetailResponse.BuyoutPrice.IsZero();

    if (canBid)
    {
        m_BtnBid.UnLock();
        m_BtnBid.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
        m_BtnBid.ChangeTextColor(RGBA(255, 255, 255, 255));
    }
    else
    {
        m_BtnBid.Lock();
        m_BtnBid.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
        m_BtnBid.ChangeTextColor(RGBA(100, 100, 100, 255));
    }

    if (canBuyout)
    {
        m_BtnBuyout.UnLock();
        m_BtnBuyout.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
        m_BtnBuyout.ChangeTextColor(RGBA(255, 255, 255, 255));
    }
    else
    {
        m_BtnBuyout.Lock();
        m_BtnBuyout.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
        m_BtnBuyout.ChangeTextColor(RGBA(100, 100, 100, 255));
    }

    m_BtnBid.Render();
    m_BtnBuyout.Render();

    if (m_bHasOperationResult)
    {
        const bool succeeded = m_LastOperationResult == AuctionResult::Success;
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(succeeded ? 120 : 255, succeeded ? 220 : 90, 120, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        const wchar_t* message = m_LastOperationMessage.empty()
            ? (succeeded ? I18N::Game::Success : I18N::Game::Failed)
            : m_LastOperationMessage.c_str();
        g_pRenderText->RenderText((float)m_Pos.x, (float)(m_Pos.y + OPERATION_RESULT_Y_OFFSET),
            message, (float)WINDOW_WIDTH, 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderPageControls()
{
    // Locked (and grayed, matching CNewUIUnitedMarketPlaceWindow's own locked-button treatment) at page 1 and
    // once the last known page is reached. Without a response yet, both read as "at the only known page".
    const bool atFirstPage = m_CurrentPage <= 1;
    const bool hasResponse = m_iCurrentTab == TAB_MAILBOX ? m_bHasMailboxResponse : m_bHasListingResponse;
    const uint16_t knownTotalPages = m_iCurrentTab == TAB_MAILBOX
        ? m_MailboxResponse.TotalPages : m_ListingResponse.TotalPages;
    const bool atLastPage = !hasResponse || m_CurrentPage >= knownTotalPages;

    if (atFirstPage)
    {
        m_BtnPrevPage.Lock();
        m_BtnPrevPage.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
        m_BtnPrevPage.ChangeTextColor(RGBA(100, 100, 100, 255));
    }
    else
    {
        m_BtnPrevPage.UnLock();
        m_BtnPrevPage.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
        m_BtnPrevPage.ChangeTextColor(RGBA(255, 255, 255, 255));
    }

    if (atLastPage)
    {
        m_BtnNextPage.Lock();
        m_BtnNextPage.ChangeImgColor(BUTTON_STATE_UP, RGBA(100, 100, 100, 255));
        m_BtnNextPage.ChangeTextColor(RGBA(100, 100, 100, 255));
    }
    else
    {
        m_BtnNextPage.UnLock();
        m_BtnNextPage.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
        m_BtnNextPage.ChangeTextColor(RGBA(255, 255, 255, 255));
    }

    m_BtnPrevPage.Render();
    m_BtnNextPage.Render();

    const uint16_t totalPages = hasResponse ? std::max<uint16_t>(knownTotalPages, 1) : 1;
    wchar_t pageText[32];
    mu_swprintf(pageText, L"%d / %d", m_CurrentPage, totalPages);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText((float)m_Pos.x, (float)(m_Pos.y + PAGE_BTN_Y_OFFSET + 4), pageText, (float)WINDOW_WIDTH, 0, RT3_SORT_CENTER);
}

bool SEASON3B::CNewUIAuctionWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

void SEASON3B::CNewUIAuctionWindow::Render3D()
{
    if (m_iCurrentTab == TAB_SELL)
    {
        if (m_bHasSellItem)
        {
            const int iconX = m_Pos.x + SELL_PANEL_X + SELL_ICON_MARGIN;
            const int iconY = m_Pos.y + SELL_PANEL_Y + (SELL_PANEL_HEIGHT - SELL_ICON_SIZE) / 2;
            RenderItem3D((float)iconX, (float)iconY, (float)SELL_ICON_SIZE, (float)SELL_ICON_SIZE,
                m_SellItemSnapshot.Type, m_SellItemSnapshot.Level, m_SellItemSnapshot.ExcellentFlags,
                m_SellItemSnapshot.AncientDiscriminator, false);
        }

        if (m_bPointingSellItem && m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->RenderUI2DEffect(INFORMATION_CAMERA_Z_ORDER, UI2DEffectCallback, this,
                static_cast<DWORD>(MaxBrowseRows + 1), 0);
        }
        return;
    }

    if (m_iCurrentTab != TAB_BROWSE && m_iCurrentTab != TAB_MY_LISTINGS && m_iCurrentTab != TAB_MAILBOX)
    {
        return;
    }

    if (m_bShowingDetail)
    {
        if (m_DetailItem != nullptr)
        {
            const int iconX = m_Pos.x + BROWSE_BODY_X;
            const int iconY = m_Pos.y + BROWSE_BODY_Y;
            RenderItem3D((float)iconX, (float)iconY, (float)DETAIL_ICON_SIZE, (float)DETAIL_ICON_SIZE,
                m_DetailItem->Type, m_DetailItem->Level, m_DetailItem->ExcellentFlags, m_DetailItem->AncientDiscriminator, false);
        }

        if (m_bPointingDetailItem && m_pNewUI3DRenderMng)
        {
            m_pNewUI3DRenderMng->RenderUI2DEffect(INFORMATION_CAMERA_Z_ORDER, UI2DEffectCallback, this, static_cast<DWORD>(MaxBrowseRows), 0);
        }

        return;
    }

    const bool hasResponse = m_iCurrentTab == TAB_MAILBOX ? m_bHasMailboxResponse : m_bHasListingResponse;
    if (!hasResponse)
    {
        return;
    }

    const size_t entryCount = m_iCurrentTab == TAB_MAILBOX
        ? m_MailboxResponse.Entries.size() : m_ListingResponse.Listings.size();
    const auto remainingRows = entryCount - std::min(m_BrowseScrollOffset, entryCount);
    const auto rowCount = std::min(MaxBrowseRows, remainingRows);
    for (size_t row = 0; row < rowCount; ++row)
    {
        const ITEM* item = m_RowItems[row];
        if (item == nullptr)
        {
            continue;
        }

        const int iconX = m_Pos.x + (m_iCurrentTab == TAB_BROWSE ? BROWSE_TABLE_X : BROWSE_BODY_X)
            + (m_iCurrentTab == TAB_MAILBOX ? MAILBOX_CONTENT_COLUMN_X : 0);
        const int iconY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT + BROWSE_ICON_MARGIN;
        RenderItem3D((float)iconX, (float)iconY, (float)BROWSE_ICON_SIZE, (float)BROWSE_ICON_SIZE,
            item->Type, item->Level, item->ExcellentFlags, item->AncientDiscriminator, false);
    }

    if (m_iCurrentTab == TAB_BROWSE && m_bHasOpenResponse)
    {
        size_t visibleCurrencyCount = 0;
        for (const auto& currency : m_OpenResponse.Currencies)
        {
            if (currency.Enabled && currency.CurrencyMode != AuctionCurrencyMode::Zen && currency.LooseItemType != 0)
            {
                ++visibleCurrencyCount;
            }
        }

        const size_t allCurrencyCount = std::count_if(m_OpenResponse.Currencies.begin(), m_OpenResponse.Currencies.end(),
            [](const auto& currency)
            {
                return currency.Enabled && (currency.CurrencyMode == AuctionCurrencyMode::Zen || currency.LooseItemType != 0);
            });
        if (visibleCurrencyCount > 0 && allCurrencyCount > 0)
        {
            const int segmentWidth = BALANCE_BAR_WIDTH / static_cast<int>(allCurrencyCount);
            int segment = 0;
            for (const auto& currency : m_OpenResponse.Currencies)
            {
                if (!currency.Enabled || (currency.CurrencyMode != AuctionCurrencyMode::Zen && currency.LooseItemType == 0))
                {
                    continue;
                }
                if (currency.CurrencyMode != AuctionCurrencyMode::Zen)
                {
                    RenderItem3D((float)(m_Pos.x + BALANCE_BAR_X + segment * segmentWidth + 3),
                        (float)(m_Pos.y + BALANCE_BAR_Y + 2), (float)BALANCE_ICON_SIZE, (float)BALANCE_ICON_SIZE,
                        currency.LooseItemType, 0, 0, 0, false);
                }
                ++segment;
            }
        }
    }

    // Deferred to the shared 3D render manager so the tooltip draws after every window's own icons, the same
    // ordering CNewUIMyInventory::Render3D relies on for its own item tooltip.
    if (m_iPointedRow != -1 && m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->RenderUI2DEffect(INFORMATION_CAMERA_Z_ORDER, UI2DEffectCallback, this, static_cast<DWORD>(m_iPointedRow), 0);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderSellTab()
{
    const int panelX = m_Pos.x + SELL_PANEL_X;
    const int panelY = m_Pos.y + SELL_PANEL_Y;

    EnableAlphaTest();
    RenderColorQuadARGB(panelX, panelY, SELL_PANEL_WIDTH, SELL_PANEL_HEIGHT, ARGB(150, 18, 18, 18));
    RenderColorQuadARGB(panelX, panelY, SELL_PANEL_WIDTH, 1, BROWSE_GRID_COLOR);
    RenderColorQuadARGB(panelX, panelY + SELL_PANEL_HEIGHT - 1, SELL_PANEL_WIDTH, 1, BROWSE_GRID_COLOR);
    RenderColorQuadARGB(panelX, panelY, 1, SELL_PANEL_HEIGHT, BROWSE_GRID_COLOR);
    RenderColorQuadARGB(panelX + SELL_PANEL_WIDTH - 1, panelY, 1, SELL_PANEL_HEIGHT, BROWSE_GRID_COLOR);
    EndRenderColor();

    g_pRenderText->SetBgColor(0, 0, 0, 0);
    if (!m_bHasSellItem)
    {
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(218, 186, 104, 255);
        g_pRenderText->RenderText((float)(panelX + 12), (float)(panelY + 22), I18N::Game::AuctionSelectItem,
            (float)(SELL_PANEL_WIDTH - 24), 0, RT3_SORT_CENTER);
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(185, 185, 185, 255);
        g_pRenderText->RenderText((float)(panelX + 12), (float)(panelY + 48), I18N::Game::AuctionSelectItemHint,
            (float)(SELL_PANEL_WIDTH - 24), 0, RT3_SORT_CENTER);
        return;
    }

    const int textX = panelX + SELL_ICON_MARGIN + SELL_ICON_SIZE + 14;
    const int textWidth = SELL_PANEL_WIDTH - (textX - panelX) - 12;
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(218, 186, 104, 255);
    g_pRenderText->RenderText((float)textX, (float)(panelY + 17), I18N::Game::AuctionSelectedItem,
        (float)textWidth, 0, RT3_SORT_LEFT);

    SetBrowseItemNameColor(&m_SellItemSnapshot);
    const std::wstring itemName = GetItemDisplayName(&m_SellItemSnapshot);
    g_pRenderText->RenderText((float)textX, (float)(panelY + 39), itemName.c_str(),
        (float)textWidth, 0, RT3_SORT_LEFT);

    const std::wstring slotText = std::wstring(I18N::Game::AuctionInventorySlot) + L": " + std::to_wstring(m_SellInventorySlot);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(175, 175, 175, 255);
    g_pRenderText->RenderText((float)textX, (float)(panelY + 61), slotText.c_str(),
        (float)textWidth, 0, RT3_SORT_LEFT);
}

void SEASON3B::CNewUIAuctionWindow::RenderSellItemTooltip() const
{
    if (!m_bHasSellItem)
    {
        return;
    }

    const int iconX = m_Pos.x + SELL_PANEL_X + SELL_ICON_MARGIN;
    const int iconY = m_Pos.y + SELL_PANEL_Y + (SELL_PANEL_HEIGHT - SELL_ICON_SIZE) / 2;
    RenderItemInfo(iconX + SELL_ICON_SIZE / 2, iconY + SELL_ICON_SIZE / 2,
        const_cast<ITEM*>(&m_SellItemSnapshot), false);
}

void SEASON3B::CNewUIAuctionWindow::RenderBrowseHeader()
{
    // EnableAlphaBlend is the engine's additive "glow" mode, not conventional alpha blending. It turns
    // translucent table panels into the opaque white/yellow blocks seen during the first in-game pass.
    EnableAlphaTest();
    RenderColorQuadARGB(m_Pos.x + BROWSE_TABLE_X, m_Pos.y + BROWSE_HEADER_Y,
        BROWSE_TABLE_CONTENT_WIDTH, BROWSE_HEADER_HEIGHT, BROWSE_HEADER_COLOR);

    const int separators[] = {
        BROWSE_LEVEL_COLUMN_X,
        BROWSE_TIME_COLUMN_X,
        BROWSE_SELLER_COLUMN_X,
        BROWSE_PRICE_COLUMN_X,
        BROWSE_BUYOUT_COLUMN_X,
    };
    for (const int separator : separators)
    {
        RenderColorQuadARGB(m_Pos.x + BROWSE_TABLE_X + separator, m_Pos.y + BROWSE_HEADER_Y,
            1, BROWSE_HEADER_HEIGHT + BROWSE_SCROLLBAR_HEIGHT, BROWSE_GRID_COLOR);
    }
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(218, 186, 104, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const float headerY = static_cast<float>(m_Pos.y + BROWSE_HEADER_Y + 5);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_ITEM_COLUMN_X + 5), headerY,
        I18N::Game::Item, (float)(BROWSE_ITEM_COLUMN_WIDTH - 10), 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_LEVEL_COLUMN_X), headerY,
        I18N::Game::Level, (float)BROWSE_LEVEL_COLUMN_WIDTH, 0, RT3_SORT_CENTER);

    std::wstring timeHeader = I18N::Game::TimeLeft;
    if (m_iCurrentTab == TAB_BROWSE && m_SelectedSort == AuctionSort::EndingSoonest)
    {
        timeHeader += L" ^";
    }
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_TIME_COLUMN_X), headerY,
        timeHeader.c_str(), (float)BROWSE_TIME_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_SELLER_COLUMN_X), headerY,
        m_iCurrentTab == TAB_MY_LISTINGS ? I18N::Game::Status : I18N::Game::Seller,
        (float)BROWSE_SELLER_COLUMN_WIDTH, 0, RT3_SORT_CENTER);

    std::wstring priceHeader = I18N::Game::AuctionPrice;
    if (m_iCurrentTab == TAB_BROWSE && m_SelectedSort == AuctionSort::PriceAscending)
    {
        priceHeader += L" ^";
    }
    else if (m_iCurrentTab == TAB_BROWSE && m_SelectedSort == AuctionSort::PriceDescending)
    {
        priceHeader += L" v";
    }
    if (m_iCurrentTab == TAB_BROWSE && m_SelectedCurrency == AuctionCurrencyMode::Fruits)
    {
        g_pRenderText->SetTextColor(110, 110, 110, 255);
    }
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_PRICE_COLUMN_X), headerY,
        priceHeader.c_str(), (float)BROWSE_PRICE_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
    g_pRenderText->SetTextColor(218, 186, 104, 255);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_BUYOUT_COLUMN_X), headerY,
        I18N::Game::Buyout, (float)BROWSE_BUYOUT_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIAuctionWindow::RenderBrowseTab()
{
    // Eight compact table rows. Item icons remain in the engine's 3D pass; the text columns and grid are
    // ordinary UI rendering. The countdown uses m_ServerClock's live estimate, not the frozen response time.
    constexpr int BodyWidth = BROWSE_TABLE_CONTENT_WIDTH;

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    if (!m_bHasListingResponse)
    {
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X), (float)(m_Pos.y + BROWSE_BODY_Y), I18N::Game::PleaseWait, (float)BodyWidth, 0, RT3_SORT_LEFT);
        return;
    }

    if (m_ListingResponse.Listings.empty())
    {
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X), (float)(m_Pos.y + BROWSE_BODY_Y), I18N::Game::NoListingsFound, (float)BodyWidth, 0, RT3_SORT_LEFT);
        return;
    }

    const uint32_t estimatedServerTime = m_ServerClock.EstimatedServerTime(GetTickCount());
    const auto remainingRows = m_ListingResponse.Listings.size() - std::min(m_BrowseScrollOffset, m_ListingResponse.Listings.size());
    const auto rowCount = std::min(MaxBrowseRows, remainingRows);
    for (size_t row = 0; row < rowCount; ++row)
    {
        const auto& listing = m_ListingResponse.Listings[m_BrowseScrollOffset + row];
        const int rowY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT;
        const bool isHovered = static_cast<int>(row) == m_iPointedRow;

        EnableAlphaTest();
        RenderColorQuadARGB(m_Pos.x + BROWSE_TABLE_X, rowY + BROWSE_ROW_PADDING,
            BodyWidth, BROWSE_ROW_HEIGHT - 2 * BROWSE_ROW_PADDING,
            isHovered ? BROWSE_CARD_HOVER_COLOR : BROWSE_CARD_COLOR);
        EndRenderColor();

        const auto remainingSeconds = listing.EndsAt > estimatedServerTime ? (listing.EndsAt - estimatedServerTime) : 0;
        const auto countdown = AuctionHouse::FormatAuctionCountdown(std::chrono::seconds(remainingSeconds));
        const std::wstring priceText = listing.CurrentPrice.IsFruitBasket()
            ? I18N::Game::FruitBasket
            : std::to_wstring(listing.CurrentPrice.Scalar());
        const std::wstring buyoutText = listing.BuyoutPrice.IsZero()
            ? L"-"
            : (listing.BuyoutPrice.IsFruitBasket() ? I18N::Game::FruitBasket : std::to_wstring(listing.BuyoutPrice.Scalar()));
        const std::wstring itemName = m_RowItems[row] != nullptr ? GetItemDisplayName(m_RowItems[row]) : std::wstring();

        const int textX = m_Pos.x + BROWSE_TABLE_X + BROWSE_TEXT_X_OFFSET;
        const int textWidth = BROWSE_ITEM_COLUMN_WIDTH - BROWSE_TEXT_X_OFFSET - 4;
        const float textY = static_cast<float>(rowY + 7);

        g_pRenderText->SetFont(g_hFontBold);
        SetBrowseItemNameColor(m_RowItems[row]);
        g_pRenderText->RenderText((float)textX, textY, itemName.c_str(), (float)textWidth, 0, RT3_SORT_LEFT);

        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(200, 200, 200, 255);
        const std::wstring levelText = L"+" + std::to_wstring(m_RowItems[row] != nullptr ? m_RowItems[row]->Level : 0);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_LEVEL_COLUMN_X), textY,
            levelText.c_str(), (float)BROWSE_LEVEL_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_TIME_COLUMN_X), textY,
            countdown.c_str(), (float)BROWSE_TIME_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_SELLER_COLUMN_X), textY,
            m_iCurrentTab == TAB_MY_LISTINGS ? ListingStatusText(listing.Status) : listing.SellerName.c_str(),
            (float)BROWSE_SELLER_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_PRICE_COLUMN_X), textY,
            priceText.c_str(), (float)BROWSE_PRICE_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_TABLE_X + BROWSE_BUYOUT_COLUMN_X), textY,
            buyoutText.c_str(), (float)BROWSE_BUYOUT_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
    }

}

void SEASON3B::CNewUIAuctionWindow::RenderCategoryRail()
{
    // A persistent category rail makes filtering a single, visible action instead of hiding the server's
    // seven real auction categories inside a dropdown. It deliberately uses the existing MU table materials
    // rather than borrowing the reference game's artwork.
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    for (int category = 0; category < static_cast<int>(std::size(m_CategoryLabels)); ++category)
    {
        const int y = m_Pos.y + CATEGORY_RAIL_Y + category * CATEGORY_RAIL_ROW_HEIGHT;
        const bool selected = category == m_SelectedCategoryIndex;
        EnableAlphaTest();
        RenderColorQuadARGB(m_Pos.x + CATEGORY_RAIL_X, y + 1, CATEGORY_RAIL_WIDTH, CATEGORY_RAIL_ROW_HEIGHT - 2,
            selected ? BROWSE_CARD_HOVER_COLOR : BROWSE_CARD_COLOR);
        EndRenderColor();

        g_pRenderText->SetTextColor(selected ? 255 : 205, selected ? 215 : 205, selected ? 100 : 205, 255);
        g_pRenderText->RenderText((float)(m_Pos.x + CATEGORY_RAIL_X + 7), (float)(y + 8),
            m_CategoryLabels[category], (float)(CATEGORY_RAIL_WIDTH - 12), 0, RT3_SORT_LEFT);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderCurrencyBalances()
{
    if (!m_bHasOpenResponse)
    {
        return;
    }

    size_t visibleCurrencyCount = 0;
    for (const auto& currency : m_OpenResponse.Currencies)
    {
        if (currency.Enabled && (currency.CurrencyMode == AuctionCurrencyMode::Zen || currency.LooseItemType != 0))
        {
            ++visibleCurrencyCount;
        }
    }
    if (visibleCurrencyCount == 0)
    {
        return;
    }

    EnableAlphaTest();
    RenderColorQuadARGB(m_Pos.x + BALANCE_BAR_X, m_Pos.y + BALANCE_BAR_Y,
        BALANCE_BAR_WIDTH, BALANCE_BAR_HEIGHT, BROWSE_HEADER_COLOR);
    EndRenderColor();

    const int segmentWidth = BALANCE_BAR_WIDTH / static_cast<int>(visibleCurrencyCount);
    int segment = 0;
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(235, 210, 120, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    for (const auto& currency : m_OpenResponse.Currencies)
    {
        if (!currency.Enabled || (currency.CurrencyMode != AuctionCurrencyMode::Zen && currency.LooseItemType == 0))
        {
            continue;
        }

        const int x = m_Pos.x + BALANCE_BAR_X + segment * segmentWidth;
        if (segment > 0)
        {
            EnableAlphaTest();
            RenderColorQuadARGB(x, m_Pos.y + BALANCE_BAR_Y, 1, BALANCE_BAR_HEIGHT, BROWSE_GRID_COLOR);
            EndRenderColor();
        }

        if (currency.CurrencyMode == AuctionCurrencyMode::Zen)
        {
            // This is the same loaded money artwork the inventory uses; cropping its left edge gives the
            // familiar coin badge without adding a duplicate asset.
            RenderImage(CNewUIMyInventory::IMAGE_INVENTORY_MONEY,
                static_cast<float>(x + 3), static_cast<float>(m_Pos.y + BALANCE_BAR_Y + 2),
                static_cast<float>(BALANCE_ICON_SIZE), static_cast<float>(BALANCE_ICON_SIZE));
        }

        const std::wstring amount = std::to_wstring(currency.Spendable.Scalar());
        g_pRenderText->RenderText((float)(x + BALANCE_ICON_SIZE + 5), (float)(m_Pos.y + BALANCE_BAR_Y + 5),
            amount.c_str(), (float)(segmentWidth - BALANCE_ICON_SIZE - 8), 0, RT3_SORT_LEFT);
        ++segment;
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderMailboxHeader()
{
    EnableAlphaTest();
    RenderColorQuadARGB(m_Pos.x + BROWSE_BODY_X, m_Pos.y + BROWSE_HEADER_Y,
        BROWSE_CONTENT_WIDTH, BROWSE_HEADER_HEIGHT, BROWSE_HEADER_COLOR);
    const int separators[] = { MAILBOX_CONTENT_COLUMN_X, MAILBOX_SOURCE_COLUMN_X, MAILBOX_STATUS_COLUMN_X };
    for (const int separator : separators)
    {
        RenderColorQuadARGB(m_Pos.x + BROWSE_BODY_X + separator, m_Pos.y + BROWSE_HEADER_Y,
            1, BROWSE_HEADER_HEIGHT + BROWSE_SCROLLBAR_HEIGHT, BROWSE_GRID_COLOR);
    }
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(218, 186, 104, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const float y = static_cast<float>(m_Pos.y + BROWSE_HEADER_Y + 5);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + 5), y, I18N::Game::Mailbox,
        (float)(MAILBOX_KIND_COLUMN_WIDTH - 10), 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_CONTENT_COLUMN_X + 5), y,
        I18N::Game::AuctionContents, (float)(MAILBOX_CONTENT_COLUMN_WIDTH - 10), 0, RT3_SORT_LEFT);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_SOURCE_COLUMN_X), y,
        I18N::Game::AuctionSource, (float)MAILBOX_SOURCE_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_STATUS_COLUMN_X), y,
        I18N::Game::AuctionStatus, (float)MAILBOX_STATUS_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIAuctionWindow::RenderMailboxTab()
{
    if (!m_bHasMailboxResponse)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)(m_Pos.y + BROWSE_BODY_Y + 8),
            I18N::Game::PleaseWait, (float)BROWSE_CONTENT_WIDTH, 0, RT3_SORT_CENTER);
        return;
    }

    if (m_MailboxResponse.Entries.empty())
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(190, 190, 190, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)(m_Pos.y + BROWSE_BODY_Y + 8),
            I18N::Game::AuctionNoCollections, (float)BROWSE_CONTENT_WIDTH, 0, RT3_SORT_CENTER);
        if (m_bHasOperationResult)
        {
            const bool succeeded = m_LastOperationResult == AuctionResult::Success;
            g_pRenderText->SetTextColor(succeeded ? 120 : 255, succeeded ? 220 : 90, 120, 255);
            const wchar_t* message = m_LastOperationMessage.empty()
                ? (succeeded ? I18N::Game::Success : I18N::Game::Failed)
                : m_LastOperationMessage.c_str();
            g_pRenderText->RenderText((float)m_Pos.x, (float)(m_Pos.y + OPERATION_RESULT_Y_OFFSET),
                message, (float)WINDOW_WIDTH, 0, RT3_SORT_CENTER);
        }
        return;
    }

    const auto remainingRows = m_MailboxResponse.Entries.size() - std::min(m_BrowseScrollOffset, m_MailboxResponse.Entries.size());
    const auto rowCount = std::min(MaxBrowseRows, remainingRows);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    for (size_t row = 0; row < rowCount; ++row)
    {
        const auto& entry = m_MailboxResponse.Entries[m_BrowseScrollOffset + row];
        const int y = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT;
        EnableAlphaTest();
        RenderColorQuadARGB(m_Pos.x + BROWSE_BODY_X, y, BROWSE_CONTENT_WIDTH, BROWSE_ROW_HEIGHT - BROWSE_ROW_PADDING,
            m_iPointedRow == static_cast<int>(row) ? BROWSE_CARD_HOVER_COLOR : BROWSE_CARD_COLOR);
        EndRenderColor();

        g_pRenderText->SetTextColor(220, 220, 220, 255);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + 5), (float)(y + 10),
            CollectionKindText(entry.CollectionKind), (float)(MAILBOX_KIND_COLUMN_WIDTH - 10), 0, RT3_SORT_LEFT);
        const auto contents = entry.HasItem ? std::wstring(I18N::Game::Item)
            : AuctionAmountText(entry.CurrencyMode, entry.Remaining);
        const int contentTextX = m_Pos.x + BROWSE_BODY_X + MAILBOX_CONTENT_COLUMN_X
            + (entry.HasItem ? BROWSE_TEXT_X_OFFSET : 5);
        g_pRenderText->RenderText((float)contentTextX, (float)(y + 10), contents.c_str(),
            (float)(MAILBOX_CONTENT_COLUMN_WIDTH - (entry.HasItem ? BROWSE_TEXT_X_OFFSET : 10)), 0, RT3_SORT_LEFT);
        wchar_t source[32];
        mu_swprintf(source, L"%llu", entry.SourceListingId);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_SOURCE_COLUMN_X), (float)(y + 10),
            source, (float)MAILBOX_SOURCE_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_STATUS_COLUMN_X), (float)(y + 10),
            CollectionStatusText(entry.CollectionStatus), (float)MAILBOX_STATUS_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
    }

    if (m_bHasOperationResult)
    {
        const bool succeeded = m_LastOperationResult == AuctionResult::Success;
        g_pRenderText->SetTextColor(succeeded ? 120 : 255, succeeded ? 220 : 90, 120, 255);
        const wchar_t* message = m_LastOperationMessage.empty()
            ? (succeeded ? I18N::Game::Success : I18N::Game::Failed)
            : m_LastOperationMessage.c_str();
        g_pRenderText->RenderText((float)m_Pos.x, (float)(m_Pos.y + OPERATION_RESULT_Y_OFFSET),
            message, (float)WINDOW_WIDTH, 0, RT3_SORT_CENTER);
    }
}

float SEASON3B::CNewUIAuctionWindow::GetLayerDepth()
{
    // Same band as the other standalone account/character windows (Master Level is 10.1): high enough to
    // win mouse-event dispatch over the mini-map and move-command window (see NewUIExpHuntWindow.cpp for
    // why that ordering matters), ordinary for a dialog-style window otherwise.
    return 10.1f;
}

int SEASON3B::CNewUIAuctionWindow::CurrentWindowWidth() const
{
    return m_iCurrentTab == TAB_SELL ? SELL_WINDOW_WIDTH : WINDOW_WIDTH;
}

void SEASON3B::CNewUIAuctionWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_AUCTION_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_AUCTION_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_AUCTION_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_AUCTION_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_AUCTION_BOTTOM, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_exit_00.tga", IMAGE_AUCTION_CLOSE_BTN, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_guild_tab04.tga", IMAGE_AUCTION_TAB_BTN, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_btn_empty_very_small.tga", IMAGE_AUCTION_PAGE_BTN, GL_LINEAR);
}

void SEASON3B::CNewUIAuctionWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_AUCTION_BACK);
    DeleteBitmap(IMAGE_AUCTION_TOP);
    DeleteBitmap(IMAGE_AUCTION_LEFT);
    DeleteBitmap(IMAGE_AUCTION_RIGHT);
    DeleteBitmap(IMAGE_AUCTION_BOTTOM);
    DeleteBitmap(IMAGE_AUCTION_CLOSE_BTN);
    DeleteBitmap(IMAGE_AUCTION_TAB_BTN);
    DeleteBitmap(IMAGE_AUCTION_PAGE_BTN);
}

void SEASON3B::CNewUIAuctionWindow::RenderFrame()
{
    const float x = static_cast<float>(m_Pos.x);
    const float y = static_cast<float>(m_Pos.y);
    const int windowWidth = CurrentWindowWidth();
    const float centerDestinationWidth = static_cast<float>(windowWidth - 2 * FRAME_CAP_WIDTH);

    // These legacy frame sprites are only 190 pixels wide. RenderImage samples destination dimensions as
    // source dimensions too, so asking it for 600 pixels reads beyond the sprite and produces a black void.
    // Stretch the subdued background, but preserve both ornate frame caps at 1:1 and stretch only their
    // plain center strip so the MU corners retain their original proportions.
    RenderImageStretch(IMAGE_AUCTION_BACK, x, y, static_cast<float>(windowWidth), static_cast<float>(WINDOW_HEIGHT),
        0.f, 0.f, static_cast<float>(FRAME_TEXTURE_WIDTH), static_cast<float>(FRAME_TEXTURE_HEIGHT));

    RenderImageStretch(IMAGE_AUCTION_TOP, x, y, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT),
        0.f, 0.f, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT));
    RenderImageStretch(IMAGE_AUCTION_TOP, x + FRAME_CAP_WIDTH, y, centerDestinationWidth, static_cast<float>(TOP_BAND_HEIGHT),
        static_cast<float>(FRAME_CAP_WIDTH), 0.f, static_cast<float>(FRAME_CENTER_WIDTH), static_cast<float>(TOP_BAND_HEIGHT));
    RenderImageStretch(IMAGE_AUCTION_TOP, x + windowWidth - FRAME_CAP_WIDTH, y,
        static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT),
        static_cast<float>(FRAME_TEXTURE_WIDTH - FRAME_CAP_WIDTH), 0.f,
        static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT));

    const float sideHeight = static_cast<float>(WINDOW_HEIGHT - TOP_BAND_HEIGHT - BOTTOM_BAND_HEIGHT);
    RenderImageStretch(IMAGE_AUCTION_LEFT, x, y + TOP_BAND_HEIGHT, static_cast<float>(SIDE_BAND_WIDTH), sideHeight,
        0.f, 0.f, static_cast<float>(SIDE_BAND_WIDTH), sideHeight);
    RenderImageStretch(IMAGE_AUCTION_RIGHT, x + windowWidth - SIDE_BAND_WIDTH, y + TOP_BAND_HEIGHT,
        static_cast<float>(SIDE_BAND_WIDTH), sideHeight, 0.f, 0.f, static_cast<float>(SIDE_BAND_WIDTH), sideHeight);

    const float bottomY = y + WINDOW_HEIGHT - BOTTOM_BAND_HEIGHT;
    RenderImageStretch(IMAGE_AUCTION_BOTTOM, x, bottomY, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT),
        0.f, 0.f, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT));
    RenderImageStretch(IMAGE_AUCTION_BOTTOM, x + FRAME_CAP_WIDTH, bottomY, centerDestinationWidth, static_cast<float>(BOTTOM_BAND_HEIGHT),
        static_cast<float>(FRAME_CAP_WIDTH), 0.f, static_cast<float>(FRAME_CENTER_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT));
    RenderImageStretch(IMAGE_AUCTION_BOTTOM, x + windowWidth - FRAME_CAP_WIDTH, bottomY,
        static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT),
        static_cast<float>(FRAME_TEXTURE_WIDTH - FRAME_CAP_WIDTH), 0.f,
        static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT));
}
