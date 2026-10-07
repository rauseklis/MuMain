// NewUIAuctionWindow.cpp: implementation of the CNewUIAuctionWindow class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "Engine/Object/ZzzInventory.h"
#include "I18N/All.h"
#include "Audio/DSPlaySound.h"
#include "Network/Server/WSclient.h"

#include <algorithm>

using namespace SEASON3B;

namespace
{
    constexpr int TAB_REGION_X = 10;
    constexpr int TAB_REGION_Y = 40;
    constexpr int TAB_WIDTH = 92;
    constexpr int TAB_HEIGHT = 26;
    constexpr int TOP_BAND_HEIGHT = 64;
    constexpr int BOTTOM_BAND_HEIGHT = 45;
    constexpr int SIDE_BAND_WIDTH = 21;
    constexpr int CLOSE_BTN_WIDTH = 36;
    constexpr int CLOSE_BTN_HEIGHT = 29;
    constexpr int CLOSE_BTN_MARGIN = 8;
    constexpr int TOOLBAR_X = 11;
    constexpr int TOOLBAR_Y = 69;
    constexpr int CURRENCY_COMBO_WIDTH = 150;
    constexpr int CURRENCY_COMBO_ITEM_HEIGHT = 22;
    constexpr int BROWSE_BODY_X = 11;
    constexpr int BROWSE_BODY_Y = 112;
    constexpr int BROWSE_ROW_HEIGHT = 32;
    constexpr int BROWSE_ICON_SIZE = 28;
    constexpr int BROWSE_ICON_MARGIN = 2;
    constexpr int BROWSE_TEXT_X_OFFSET = BROWSE_ICON_SIZE + 6;
}

SEASON3B::CNewUIAuctionWindow::CNewUIAuctionWindow()
    : m_pNewUIMng(nullptr), m_pNewUI3DRenderMng(nullptr), m_Pos{ 0, 0 }, m_iCurrentTab(TAB_BROWSE),
      m_bOpenRequestPending(false), m_PendingOpenRequestId(0), m_bHasOpenResponse(false),
      m_SelectedCurrency(AuctionCurrencyMode::Zen), m_CurrentPage(1), m_SelectedSort(AuctionSort::EndingSoonest),
      m_bBrowseRequestPending(false), m_PendingBrowseRequestId(0), m_bHasBrowseResponse(false),
      m_iPointedRow(-1)
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
    m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);

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
    m_CurrencyCombo.Setup(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, CURRENCY_COMBO_WIDTH, CURRENCY_COMBO_ITEM_HEIGHT,
        m_CurrencyLabels, 9, static_cast<int>(m_SelectedCurrency));

    Show(false);

    return true;
}

void SEASON3B::CNewUIAuctionWindow::Release()
{
    UnloadImages();
    ReleaseRowItems();

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

void SEASON3B::CNewUIAuctionWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
}

void SEASON3B::CNewUIAuctionWindow::OpeningProcess()
{
    // Reset to Browse every time the window opens, and ask the server for the current fees/currencies/
    // pending Mailbox count. The design spec's "loading state until the response arrives" (4.1) is UI that
    // belongs to the Browse tab (task 4.3), which does not exist yet; m_bHasOpenResponse tracks whether one
    // has arrived, for that tab to read once it does.
    m_iCurrentTab = TAB_BROWSE;
    m_TabBtn.ChangeFrame(m_iCurrentTab);

    m_bHasOpenResponse = false;
    m_PendingOpenRequestId = AuctionHouse::NextAuctionRequestId();
    m_bOpenRequestPending = true;
    SocketClient->ToGameServer()->SendAuctionOpenRequest(m_PendingOpenRequestId);

    m_CurrentPage = 1;
    SendBrowseRequest();
}

void SEASON3B::CNewUIAuctionWindow::SendBrowseRequest()
{
    // V1 scope: only currency, category, sort and page are honored server-side (confirmed by reading
    // AuctionBrowseHandlerPlugIn.cs). Category 255 means "every category" (the server's own sentinel,
    // AuctionBrowseHandlerPlugIn.AllCategories). The remaining fields are sent as the widest possible
    // range, so they behave as "no filter" whenever the server starts honoring them.
    constexpr BYTE AllCategories = 0xFF;
    m_bHasBrowseResponse = false;
    m_PendingBrowseRequestId = AuctionHouse::NextAuctionRequestId();
    m_bBrowseRequestPending = true;
    SocketClient->ToGameServer()->SendAuctionBrowseRequest(
        m_PendingBrowseRequestId,
        m_CurrentPage,
        AllCategories,
        m_SelectedCurrency,
        m_SelectedSort,
        0xFFFFFFFFu, // class mask: every class
        0, // level minimum
        255, // level maximum
        0, // option flags: none required
        0, 0, 0, 0, 0, 0, // minimum price, every component: no floor
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, // maximum price, every component: no ceiling
        0, // max remaining hours: no limit
        0, nullptr, 0); // name search: none
}

void SEASON3B::CNewUIAuctionWindow::SetBrowseResponse(const AuctionHouse::AuctionBrowseResponse& response)
{
    if (!m_bBrowseRequestPending || response.RequestId != m_PendingBrowseRequestId)
    {
        return;
    }

    m_bBrowseRequestPending = false;
    m_bHasBrowseResponse = true;
    m_BrowseResponse = response;
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
    RebuildRowItems();
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

    const auto rowCount = std::min(MaxBrowseRows, m_BrowseResponse.Listings.size());
    for (size_t row = 0; row < rowCount; ++row)
    {
        const auto& listing = m_BrowseResponse.Listings[row];
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

    const int iconX = m_Pos.x + BROWSE_BODY_X;
    const int iconY = m_Pos.y + BROWSE_BODY_Y + row * BROWSE_ROW_HEIGHT + BROWSE_ICON_MARGIN;
    RenderItemInfo(iconX + BROWSE_ICON_SIZE / 2, iconY + BROWSE_ICON_SIZE / 2, m_RowItems[row], false);
}

void SEASON3B::CNewUIAuctionWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD /*dwParamB*/)
{
    if (pClass != nullptr)
    {
        static_cast<CNewUIAuctionWindow*>(pClass)->RenderRowItemTooltip(static_cast<int>(dwParamA));
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
    m_ServerClock.Sync(response.ServerTime, GetTickCount());
}

void SEASON3B::CNewUIAuctionWindow::ClosingProcess()
{
    // The Sell tab will own closing an inventory window it opened for itself (design spec 4.2); there is
    // nothing to release yet since the Sell tab has no content.
}

bool SEASON3B::CNewUIAuctionWindow::UpdateMouseEvent()
{
    if (BtnProcess())
    {
        return false;
    }

    if (CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
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

    // The combo's own contract: its expanded dropdown can extend past this widget's own small hit box, so
    // the owner must separately treat IsMouseOverWidget() as a consumed click.
    if (m_iCurrentTab == TAB_BROWSE && m_CurrencyCombo.IsMouseOverWidget())
    {
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIAuctionWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_AUCTION_HOUSE) == true)
    {
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
        if (selected != RADIOGROUPEVENT_NONE)
        {
            m_iCurrentTab = selected;
        }

        if (m_iCurrentTab == TAB_BROWSE && m_CurrencyCombo.UpdateMouseEvent())
        {
            m_SelectedCurrency = static_cast<AuctionCurrencyMode>(m_CurrencyCombo.GetSelectedIndex());
            m_CurrentPage = 1;
            SendBrowseRequest();
        }

        m_iPointedRow = -1;
        if (m_iCurrentTab == TAB_BROWSE && m_bHasBrowseResponse)
        {
            const auto rowCount = std::min(MaxBrowseRows, m_BrowseResponse.Listings.size());
            for (size_t row = 0; row < rowCount; ++row)
            {
                if (CheckMouseIn(m_Pos.x + BROWSE_BODY_X, m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT,
                    WINDOW_WIDTH - 2 * BROWSE_BODY_X, BROWSE_ROW_HEIGHT))
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

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText((float)(m_Pos.x + 15), (float)(m_Pos.y + 13), I18N::Game::AuctionHouse, (float)(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);

    if (m_iCurrentTab == TAB_BROWSE)
    {
        RenderBrowseTab();

        // Rendered last, per CNewUIComboBox's own contract, so its expanded dropdown draws on top of the row text.
        m_CurrencyCombo.Render();
    }

    DisableAlphaBlend();

    return true;
}

bool SEASON3B::CNewUIAuctionWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

void SEASON3B::CNewUIAuctionWindow::Render3D()
{
    if (m_iCurrentTab != TAB_BROWSE || !m_bHasBrowseResponse)
    {
        return;
    }

    const auto rowCount = std::min(MaxBrowseRows, m_BrowseResponse.Listings.size());
    for (size_t row = 0; row < rowCount; ++row)
    {
        const ITEM* item = m_RowItems[row];
        if (item == nullptr)
        {
            continue;
        }

        const int iconX = m_Pos.x + BROWSE_BODY_X;
        const int iconY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT + BROWSE_ICON_MARGIN;
        RenderItem3D((float)iconX, (float)iconY, (float)BROWSE_ICON_SIZE, (float)BROWSE_ICON_SIZE,
            item->Type, item->Level, item->ExcellentFlags, item->AncientDiscriminator, false);
    }

    // Deferred to the shared 3D render manager so the tooltip draws after every window's own icons, the same
    // ordering CNewUIMyInventory::Render3D relies on for its own item tooltip.
    if (m_iPointedRow != -1 && m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, UI2DEffectCallback, this, static_cast<DWORD>(m_iPointedRow), 0);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderBrowseTab()
{
    // Main body region per the design spec's shared layout table (4.2): x 31-409, y 137-416 in the 640x480
    // logical canvas, i.e. local offset (11, 112) from this window's own (20, 25) origin. Eight rows. Each
    // row's item icon is drawn in Render3D() (the engine's 3D pass, same as inventory slots), so the text
    // columns here start after BROWSE_TEXT_X_OFFSET to leave room for it. The countdown uses m_ServerClock's
    // live estimate, not the frozen ServerTime the last response carried, so it ticks down between responses.
    constexpr int BodyWidth = WINDOW_WIDTH - 2 * BROWSE_BODY_X;

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    if (!m_bHasBrowseResponse)
    {
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)(m_Pos.y + BROWSE_BODY_Y), I18N::Game::PleaseWait, (float)BodyWidth, 0, RT3_SORT_LEFT);
        return;
    }

    if (m_BrowseResponse.Listings.empty())
    {
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X), (float)(m_Pos.y + BROWSE_BODY_Y), I18N::Game::NoListingsFound, (float)BodyWidth, 0, RT3_SORT_LEFT);
        return;
    }

    const uint32_t estimatedServerTime = m_ServerClock.EstimatedServerTime(GetTickCount());
    const auto rowCount = std::min(MaxBrowseRows, m_BrowseResponse.Listings.size());
    for (size_t row = 0; row < rowCount; ++row)
    {
        const auto& listing = m_BrowseResponse.Listings[row];
        const auto remainingSeconds = listing.EndsAt > estimatedServerTime ? (listing.EndsAt - estimatedServerTime) : 0;
        const auto countdown = AuctionHouse::FormatAuctionCountdown(std::chrono::seconds(remainingSeconds));
        const std::wstring priceText = listing.CurrentPrice.IsFruitBasket()
            ? L"(fruits)"
            : std::to_wstring(listing.CurrentPrice.Scalar());

        wchar_t line[256];
        mu_swprintf(line, L"%ls   %ls   x%d   %ls", priceText.c_str(), countdown.c_str(), listing.BidCount, listing.SellerName.c_str());

        const int textX = m_Pos.x + BROWSE_BODY_X + BROWSE_TEXT_X_OFFSET;
        const int textY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT;
        g_pRenderText->RenderText((float)textX, (float)textY, line, (float)(BodyWidth - BROWSE_TEXT_X_OFFSET), 0, RT3_SORT_LEFT);
    }
}

float SEASON3B::CNewUIAuctionWindow::GetLayerDepth()
{
    // Same band as the other standalone account/character windows (Master Level is 10.1): high enough to
    // win mouse-event dispatch over the mini-map and move-command window (see NewUIExpHuntWindow.cpp for
    // why that ordering matters), ordinary for a dialog-style window otherwise.
    return 10.1f;
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
}

void SEASON3B::CNewUIAuctionWindow::RenderFrame()
{
    RenderImage(IMAGE_AUCTION_BACK, (float)m_Pos.x, (float)m_Pos.y, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    RenderImage(IMAGE_AUCTION_TOP, (float)m_Pos.x, (float)m_Pos.y, (float)WINDOW_WIDTH, (float)TOP_BAND_HEIGHT);
    RenderImage(IMAGE_AUCTION_LEFT, (float)m_Pos.x, (float)(m_Pos.y + TOP_BAND_HEIGHT), (float)SIDE_BAND_WIDTH, (float)(WINDOW_HEIGHT - TOP_BAND_HEIGHT - BOTTOM_BAND_HEIGHT));
    RenderImage(IMAGE_AUCTION_RIGHT, (float)(m_Pos.x + WINDOW_WIDTH - SIDE_BAND_WIDTH), (float)(m_Pos.y + TOP_BAND_HEIGHT), (float)SIDE_BAND_WIDTH, (float)(WINDOW_HEIGHT - TOP_BAND_HEIGHT - BOTTOM_BAND_HEIGHT));
    RenderImage(IMAGE_AUCTION_BOTTOM, (float)m_Pos.x, (float)(m_Pos.y + WINDOW_HEIGHT - BOTTOM_BAND_HEIGHT), (float)WINDOW_WIDTH, (float)BOTTOM_BAND_HEIGHT);
}
