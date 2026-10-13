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
#include "UI/NewUI/Widgets/NineSlice.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/Scaling/UITransform.h"

#include <algorithm>
#include <cmath>

using namespace SEASON3B;
using namespace UI::AuctionChrome;

namespace
{
    constexpr int TAB_REGION_X = 10;
    constexpr int TAB_REGION_Y = 40;
    // A compact, equal-width tab cluster leaves the title bar's close control unobstructed. The widest label
    // ("My Listings") still has symmetrical horizontal padding, and every tab shares this exact width.
    constexpr int TAB_WIDTH = 88;
    // Treat the four views as one compact navigation group. The old 14-pixel gaps made four related
    // choices look like isolated buttons and wasted the most legible part of the window.
    constexpr int TAB_GAP = 6;
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
    // Whether the server's create packet can carry a buy-now (mixed-price) listing yet. It cannot until the
    // mixed-price protocol lands; the Sell tab shows the mode but refuses to send it.
    constexpr bool BUY_NOW_SUPPORTED = false;
    constexpr int BROWSE_BODY_X = 11;
    constexpr int BROWSE_SCROLLBAR_RESERVED_WIDTH = 27;
    constexpr int TOOLBAR_X = 11;
    constexpr int TOOLBAR_Y = 73;
    constexpr int TOOLBAR_FIELD_HEIGHT = 23;
    // Glyph tops sit this far below a toolbar field's top edge. The UI font's text box is about 8 logical
    // pixels tall (not the 14 it looks like at the 2x screen scale), so centring it in a 23-pixel field puts the
    // top at 7. Typed text, placeholder, the "Lv." label and the range dash all share it so they align.
    constexpr int TOOLBAR_TEXT_Y_OFFSET = 7;
    constexpr int TOOLBAR_FIELD_TEXT_INSET = 6;
    constexpr int LEVEL_FIELD_TEXT_INSET = 5;
    constexpr int CURRENCY_COMBO_ITEM_HEIGHT = TOOLBAR_FIELD_HEIGHT;
    // The category rail replaced the category dropdown on screen; the combo only remembers the selection now.
    constexpr int CATEGORY_COMBO_X_OFFSET = 98;
    constexpr int CATEGORY_COMBO_WIDTH = 210;
    constexpr int BROWSE_TABLE_X = 132;
    constexpr int SEARCH_INPUT_X_OFFSET = 0;
    constexpr int SEARCH_INPUT_Y_OFFSET = TOOLBAR_Y + TOOLBAR_TEXT_Y_OFFSET;
    constexpr int SEARCH_INPUT_HEIGHT = 16;
    // Equal widths so Search/Reset read as a matched pair (not two arbitrarily-different sizes), and wide
    // enough that "Reset" (the longer-looking word at this font) doesn't clip against its own border.
    constexpr int SEARCH_BUTTON_WIDTH = 56;
    constexpr int RESET_BUTTON_WIDTH = 56;
    constexpr int SEARCH_INPUT_WIDTH = 190;
    constexpr int SEARCH_BUTTON_X_OFFSET = SEARCH_INPUT_X_OFFSET + SEARCH_INPUT_WIDTH + 6;
    constexpr int RESET_BUTTON_X_OFFSET = SEARCH_BUTTON_X_OFFSET + SEARCH_BUTTON_WIDTH + 5;
    constexpr int LEVEL_LABEL_X_OFFSET = RESET_BUTTON_X_OFFSET + RESET_BUTTON_WIDTH + 12;
    constexpr int LEVEL_LABEL_WIDTH = 22;
    constexpr int LEVEL_INPUT_WIDTH = 34;
    constexpr int LEVEL_RANGE_SEPARATOR_WIDTH = 10;
    constexpr int LEVEL_MINIMUM_X_OFFSET = LEVEL_LABEL_X_OFFSET + LEVEL_LABEL_WIDTH;
    constexpr int LEVEL_MAXIMUM_X_OFFSET = LEVEL_MINIMUM_X_OFFSET + LEVEL_INPUT_WIDTH + LEVEL_RANGE_SEPARATOR_WIDTH;
    // The currency combo takes whatever is left, so the toolbar's right edge lines up with the results table's
    // (window width minus the body margin and the scrollbar gutter) rather than stopping short of it.
    constexpr int CURRENCY_COMBO_X_OFFSET = LEVEL_MAXIMUM_X_OFFSET + LEVEL_INPUT_WIDTH + 12;
    constexpr int CURRENCY_COMBO_WIDTH = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - BROWSE_BODY_X
        - BROWSE_SCROLLBAR_RESERVED_WIDTH - TOOLBAR_X - CURRENCY_COMBO_X_OFFSET;
    constexpr size_t SEARCH_PACKET_CAPACITY = 32;
    constexpr BYTE ALL_CURRENCIES = 0xFF;
    static_assert(CURRENCY_COMBO_WIDTH >= 90, "Auction toolbar controls exceed the toolbar width");
    constexpr int BROWSE_HEADER_Y = 105;
    constexpr int BROWSE_HEADER_HEIGHT = 20;
    constexpr int BROWSE_BODY_Y = BROWSE_HEADER_Y + BROWSE_HEADER_HEIGHT;
    constexpr int BROWSE_ROW_HEIGHT = 29;
    constexpr int BROWSE_ICON_CELL_SIZE = 27;
    constexpr int BROWSE_ICON_SIZE = 23;
    constexpr int BROWSE_ICON_MARGIN = (BROWSE_ICON_CELL_SIZE - BROWSE_ICON_SIZE) / 2;
    // The slot sits one pixel inside its row; the 3D icon rectangle, the slot's hit-test and its tooltip
    // anchor all derive from this same origin so the item is centred in the slot it is drawn in.
    constexpr int BROWSE_ICON_CELL_OFFSET = 1;
    // Share of the icon area the item's measured silhouette occupies on its tightest axis. The silhouette is
    // measured, not estimated, so this can be close to 1 and still guarantee the item stays inside the slot.
    constexpr float BROWSE_ICON_FILL = 0.9f;
    constexpr int BROWSE_TEXT_X_OFFSET = BROWSE_ICON_CELL_SIZE + 6;
    // Keep the native scrollbar inside the content gutter. Its original position sat against the ornate
    // frame edge, which made the chunky legacy thumb look clipped rather than intentionally contained.
    constexpr int BROWSE_SCROLLBAR_RIGHT_MARGIN = 25;
    constexpr int BROWSE_SCROLLBAR_TOP_INSET = 5;
    // Nine full rows fit above the native frame's footer; all remaining rows in the server page are reached
    // with the existing scrollbar.
    constexpr int BROWSE_TABLE_HEIGHT = BROWSE_ROW_HEIGHT * 9;
    constexpr int BROWSE_SCROLLBAR_HEIGHT = BROWSE_TABLE_HEIGHT - BROWSE_SCROLLBAR_TOP_INSET * 2;
    constexpr int BROWSE_CONTENT_WIDTH = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - 2 * BROWSE_BODY_X - BROWSE_SCROLLBAR_RESERVED_WIDTH;
    constexpr int BROWSE_TABLE_CONTENT_WIDTH = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH - BROWSE_TABLE_X - BROWSE_BODY_X - BROWSE_SCROLLBAR_RESERVED_WIDTH;
    // The listing tables are one rounded container holding a header and a stack of row cards. The cards sit
    // TABLE_INSET inside the container; the item slot overlaps each card's left end so the two read as one piece.
    constexpr int TABLE_INSET = 3;
    constexpr int TABLE_BOTTOM_PAD = 2;
    constexpr int TABLE_CONTAINER_HEIGHT = BROWSE_HEADER_HEIGHT + BROWSE_TABLE_HEIGHT + TABLE_BOTTOM_PAD;
    // Right-anchored columns: the Item column takes whatever the table has left, so Browse (beside the category
    // rail) and My Listings (full width) share one set of columns and only the name column differs. The two
    // price columns are wide enough for a price of up to five components on two lines.
    constexpr int TABLE_TIME_COLUMN_WIDTH = 56;
    constexpr int TABLE_STATUS_COLUMN_WIDTH = 54;
    constexpr int TABLE_PRICE_COLUMN_WIDTH = 104;
    constexpr int TABLE_BUYOUT_COLUMN_WIDTH = 104;
    // Where the row card starts, relative to the table's inner left edge: the middle of the slot, so the slot
    // covers the card's rounded left end.
    constexpr int ROW_CARD_START = BROWSE_ICON_CELL_OFFSET + BROWSE_ICON_CELL_SIZE / 2;
    // Text tops inside a row: single-line cells use BROWSE_ROW_TEXT_Y_OFFSET; the item cell's two lines (name over
    // seller) straddle it.
    constexpr int ROW_NAME_Y_OFFSET = 4;
    constexpr int ROW_SUB_Y_OFFSET = 15;

    // Where a listing table sits and where its columns fall. Column positions are relative to the inner left
    // edge (innerX), where the cards start.
    struct TableLayout
    {
        int x = 0;
        int width = 0;
        bool statusColumn = false;

        int innerX() const { return x + TABLE_INSET; }
        int innerWidth() const { return width - 2 * TABLE_INSET; }
        int statusWidth() const { return statusColumn ? TABLE_STATUS_COLUMN_WIDTH : 0; }
        int buyoutX() const { return innerWidth() - TABLE_BUYOUT_COLUMN_WIDTH; }
        int priceX() const { return buyoutX() - TABLE_PRICE_COLUMN_WIDTH; }
        int statusX() const { return priceX() - statusWidth(); }
        int timeX() const { return statusX() - TABLE_TIME_COLUMN_WIDTH; }
        int itemWidth() const { return timeX(); }
    };

    TableLayout GetTableLayout(bool hasCategoryRail, bool statusColumn = false)
    {
        return hasCategoryRail ? TableLayout{ BROWSE_TABLE_X, BROWSE_TABLE_CONTENT_WIDTH, statusColumn }
                               : TableLayout{ BROWSE_BODY_X, BROWSE_CONTENT_WIDTH, statusColumn };
    }
    static_assert(BROWSE_TABLE_CONTENT_WIDTH - 2 * TABLE_INSET - TABLE_TIME_COLUMN_WIDTH - TABLE_PRICE_COLUMN_WIDTH
        - TABLE_BUYOUT_COLUMN_WIDTH >= 140, "Auction browse columns exceed their content region");
    constexpr int CATEGORY_RAIL_X = BROWSE_BODY_X;
    constexpr int CATEGORY_RAIL_Y = BROWSE_HEADER_Y;
    constexpr int CATEGORY_RAIL_WIDTH = BROWSE_TABLE_X - BROWSE_BODY_X - 7;
    constexpr int CATEGORY_RAIL_ROW_HEIGHT = 19;
    constexpr int CATEGORY_RAIL_ROW_INSET = 1;
    // Keep the character's spending resources where a buyer naturally looks for them: a compact left rail
    // below the categories. Jewels sit in a grid, stat fruits in one row, then Zen. A jewel or fruit card is
    // an icon with its count underneath and no caption: players know their jewels and fruits on sight, and
    // the room goes to the count, which has to stay readable once a jewel bank holds thousands.
    constexpr int WALLET_X = CATEGORY_RAIL_X;
    constexpr int WALLET_Y = CATEGORY_RAIL_Y + CATEGORY_RAIL_ROW_HEIGHT * 8 + 4;
    constexpr int WALLET_WIDTH = CATEGORY_RAIL_WIDTH;
    constexpr int WALLET_PANEL_GAP = 2;
    constexpr int WALLET_PANEL_PAD_TOP = 4;
    constexpr int WALLET_PANEL_PAD_BOTTOM = 4;
    // Four jewels per row leaves each card 28 pixels, enough for "99.9k" in the bold font; fruits are at most
    // five and share one row.
    constexpr int WALLET_JEWEL_COLUMNS = 4;
    constexpr int WALLET_FRUIT_COLUMNS = 5;
    // Every icon in a section is fitted into the same square, so models of different sizes still share one
    // centre line and one visual size. Both squares are centred in the same 14-pixel band above the count, so
    // the counts line up across panels. Jewels are round and fill their square, so theirs is smaller.
    constexpr int WALLET_ICON_ZONE = 14;
    constexpr int WALLET_JEWEL_ICON_SIZE = 12;
    constexpr int WALLET_FRUIT_ICON_SIZE = 14;
    constexpr float WALLET_ICON_FILL = 0.92f;
    constexpr int WALLET_NUMBER_GAP = 1;
    constexpr int WALLET_NUMBER_HEIGHT = 8;
    constexpr int WALLET_NUMBER_SIDE_PAD = 1;
    constexpr int WALLET_ROW_CONTENT_HEIGHT = WALLET_ICON_ZONE + WALLET_NUMBER_GAP + WALLET_NUMBER_HEIGHT;
    constexpr int WALLET_ROW_PITCH = WALLET_ROW_CONTENT_HEIGHT + 1;
    constexpr int WALLET_ZEN_HEIGHT = 30;
    // Zen is a title and one value centred between the panel's rings.
    constexpr int WALLET_ZEN_TITLE_Y_OFFSET = 4;
    constexpr int WALLET_ZEN_VALUE_Y_OFFSET = 18;
    // A price is shown as chips: a small icon (the same 3D jewel/fruit model the wallet uses) followed by the
    // amount, or just "<amount> Zen". Up to five components share one price cell over at most two lines.
    constexpr int PRICE_ICON_SIZE = 11;
    constexpr float PRICE_ICON_FILL = 0.92f;
    constexpr int PRICE_ICON_GAP = 2;
    constexpr int PRICE_CHIP_GAP = 6;
    constexpr int PRICE_LINE_HEIGHT = 12;
    constexpr int PRICE_CELL_PAD = 3;
    constexpr int PRICE_MAX_LINES = 2;
    constexpr int PRICE_ICON_Y_OFFSET = 0;
    // Top of the text line inside a table row; shared by Browse, My Listings and Mailbox so they agree.
    constexpr int BROWSE_ROW_TEXT_Y_OFFSET = 9;
    constexpr int MAILBOX_KIND_COLUMN_WIDTH = 145;
    constexpr int MAILBOX_CONTENT_COLUMN_X = MAILBOX_KIND_COLUMN_WIDTH;
    constexpr int MAILBOX_CONTENT_COLUMN_WIDTH = 245;
    constexpr int MAILBOX_SOURCE_COLUMN_X = MAILBOX_CONTENT_COLUMN_X + MAILBOX_CONTENT_COLUMN_WIDTH;
    constexpr int MAILBOX_SOURCE_COLUMN_WIDTH = 80;
    constexpr int MAILBOX_STATUS_COLUMN_X = MAILBOX_SOURCE_COLUMN_X + MAILBOX_SOURCE_COLUMN_WIDTH;
    constexpr int MAILBOX_STATUS_COLUMN_WIDTH = BROWSE_CONTENT_WIDTH - MAILBOX_STATUS_COLUMN_X;

    std::wstring FormatWalletAmount(uint32_t amount)
    {
        std::wstring result = std::to_wstring(amount);
        for (int separator = static_cast<int>(result.size()) - 3; separator > 0; separator -= 3)
        {
            result.insert(static_cast<size_t>(separator), 1, L',');
        }

        return result;
    }

    BYTE ReadAuctionLevelInput(CUITextInputBox& input, BYTE fallback)
    {
        wchar_t text[4]{};
        input.GetText(text, static_cast<int>(std::size(text)));
        if (text[0] == L'\0')
        {
            return fallback;
        }

        const unsigned long parsed = wcstoul(text, nullptr, 10);
        return static_cast<BYTE>(std::min(parsed, 255ul));
    }
    // Browse is the data-dense focal area. Keep its chrome quiet and black so item art and rarity colour
    // do the work, instead of letting the stone frame texture compete with every row.
    // Row cards: a dark steel ring at rest, gold when pointed at, with a faint gold wash inside.
    constexpr unsigned int ROW_HOVER_WASH = 0x22C8A45Au;
    constexpr unsigned int BROWSE_HEADER_LINE_COLOR = 0x30FFFFFFu;
    constexpr unsigned int BROWSE_GRID_COLOR = 0x50C8A45Au;
    constexpr int PAGE_BTN_WIDTH = 53;
    constexpr int PAGE_BTN_HEIGHT = 23;
    constexpr int PAGE_BTN_MARGIN_X = 20;
    constexpr int PAGE_BTN_Y_OFFSET = SEASON3B::CNewUIAuctionWindow::WINDOW_HEIGHT - BOTTOM_BAND_HEIGHT
        + (BOTTOM_BAND_HEIGHT - PAGE_BTN_HEIGHT) / 2;
    // Previous/Next are a compact pair left-aligned to the table's own left edge (not spread to the window's
    // own edges, which is where Bid/Buyout/Cancel/Collect still anchor - a different, detail-panel context),
    // leaving the rest of the bottom bar as a reserved blank panel, matching the reference layout exactly.
    constexpr int PAGE_CLUSTER_GAP = 40;
    constexpr int PAGE_CLUSTER_WIDTH = PAGE_BTN_WIDTH * 2 + PAGE_CLUSTER_GAP;
    // Page label top inside the 23-pixel button row; matches the buttons' own centred labels.
    constexpr int PAGE_LABEL_Y_OFFSET = 7;
    // The pager lines up with the left edge of whichever table is showing.
    int PageClusterX(int tab)
    {
        return tab == SEASON3B::CNewUIAuctionWindow::TAB_BROWSE ? BROWSE_TABLE_X : BROWSE_BODY_X;
    }
    int PageNextButtonX(int tab)
    {
        return PageClusterX(tab) + PAGE_BTN_WIDTH + PAGE_CLUSTER_GAP;
    }
    // X of the first item slot in a table row: Browse's table, My Listings' full-width table, or the Mailbox
    // contents column.
    int RowSlotX(int tab)
    {
        switch (tab)
        {
        case SEASON3B::CNewUIAuctionWindow::TAB_BROWSE: return BROWSE_TABLE_X + TABLE_INSET;
        case SEASON3B::CNewUIAuctionWindow::TAB_MAILBOX: return BROWSE_BODY_X + MAILBOX_CONTENT_COLUMN_X;
        default: return BROWSE_BODY_X + TABLE_INSET;
        }
    }
    constexpr int DETAIL_LINE_HEIGHT = 18;
    // The detail views (a listing, a Mailbox collection) are stacks of rounded cards under the Back button.
    constexpr int DETAIL_CARD_WIDTH = BROWSE_CONTENT_WIDTH;
    constexpr int DETAIL_CARD_Y = BROWSE_HEADER_Y;
    constexpr int DETAIL_CARD_HEIGHT = 96;
    constexpr int DETAIL_CARD_GAP = 8;
    constexpr int DETAIL_SLOT_X = BROWSE_BODY_X + 8;
    constexpr int DETAIL_SLOT_Y = DETAIL_CARD_Y + 8;
    constexpr int DETAIL_SLOT_SIZE = 80;
    constexpr int DETAIL_ICON_INSET = 4;
    constexpr int DETAIL_TEXT_X = DETAIL_SLOT_X + DETAIL_SLOT_SIZE + 12;
    constexpr int DETAIL_PRICE_CARD_Y = DETAIL_CARD_Y + DETAIL_CARD_HEIGHT + DETAIL_CARD_GAP;
    constexpr int DETAIL_PRICE_CARD_HEIGHT = 96;
    // Where a price block's chip row sits inside its card; the chip layout centres two lines on a 29-pixel row.
    constexpr int DETAIL_CHIP_ROW_Y = 33;
    constexpr int AMOUNT_CARD_Y = DETAIL_CARD_Y + DETAIL_CARD_HEIGHT + DETAIL_CARD_GAP;
    constexpr int AMOUNT_CARD_HEIGHT = 62;
    constexpr int AMOUNT_CHIP_ROW_Y = 26;
    constexpr int CLAIM_CARD_Y = AMOUNT_CARD_Y + AMOUNT_CARD_HEIGHT + DETAIL_CARD_GAP;
    constexpr int CLAIM_CARD_HEIGHT = 64;
    constexpr int BACK_BTN_WIDTH = 53;
    constexpr int BACK_BTN_HEIGHT = 23;
    constexpr int OPERATION_RESULT_Y_OFFSET = PAGE_BTN_Y_OFFSET - DETAIL_LINE_HEIGHT - 2;
    constexpr int CLAIM_INPUT_WIDTH = 58;
    constexpr int CLAIM_INPUT_HEIGHT = 16;
    constexpr int CLAIM_INPUT_COLUMN_WIDTH = 105;
    // Each claim amount sits in a recessed field: label above, field below, text inset from the frame.
    constexpr int CLAIM_LABEL_Y = CLAIM_CARD_Y + 8;
    constexpr int CLAIM_FIELD_Y = CLAIM_CARD_Y + 30;
    constexpr int CLAIM_FIELD_INSET = 6;
    constexpr int CLAIM_FIELD_WIDTH = CLAIM_INPUT_WIDTH + 2 * CLAIM_FIELD_INSET;
    constexpr int CLAIM_INPUT_Y = CLAIM_FIELD_Y + 7;
    constexpr int CLAIM_FIELD_X = BROWSE_BODY_X + 8;
    constexpr int CLAIM_INPUT_X = CLAIM_FIELD_X + CLAIM_FIELD_INSET;
    // The stock inventory is 190 wide with its right edge on the 640-unit reference canvas; Sell docks to its
    // left, a few units clear of it.
    constexpr int SELL_INVENTORY_GAP = 10;

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

    bool IsAllCurrencies(AuctionCurrencyMode currency)
    {
        return static_cast<BYTE>(currency) == ALL_CURRENCIES;
    }

    // Finds the Open response's descriptor for a jewel currency with a real item identity to render as an
    // icon. Returns null for Zen (no single item represents money) and Fruits (a five-component basket has no
    // one icon), and for any currency the Open response has not reported yet (e.g. before it has arrived, or a
    // mode the server configuration leaves disabled).
    const AuctionHouse::AuctionCurrencyDescriptor* FindJewelCurrencyDescriptor(
        const AuctionHouse::AuctionOpenResponse& openResponse, AuctionCurrencyMode currency)
    {
        if (currency == AuctionCurrencyMode::Zen || currency == AuctionCurrencyMode::Fruits)
        {
            return nullptr;
        }

        for (const auto& descriptor : openResponse.Currencies)
        {
            if (descriptor.CurrencyMode == currency && descriptor.Enabled && descriptor.LooseItemType != 0)
            {
                return &descriptor;
            }
        }

        return nullptr;
    }

    const DWORD TABLE_CONTAINER_TINT = RGBA(104, 110, 122, 255);
    const DWORD TAB_TINT_ACTIVE = RGBA(255, 214, 118, 255);
    const DWORD TAB_TINT_HOVER = RGBA(206, 212, 224, 255);
    const DWORD TAB_TINT_IDLE = RGBA(150, 156, 168, 255);
    const DWORD CATEGORY_TINT_SELECTED = RGBA(255, 214, 118, 255);
    const DWORD CATEGORY_TINT_HOVER = RGBA(206, 212, 224, 255);
    const DWORD CATEGORY_TINT_IDLE = RGBA(118, 124, 136, 255);
    constexpr int TAB_BADGE_GAP = 4;

    // True over the whole skinned toolbar field (frame included), not only the text input inside it.
    bool IsMouseOverToolbarField(int windowX, int windowY, int fieldOffset, int fieldWidth)
    {
        return SEASON3B::CheckMouseIn(windowX + TOOLBAR_X + fieldOffset, windowY + TOOLBAR_Y, fieldWidth, TOOLBAR_FIELD_HEIGHT);
    }

    bool IsMouseOverAnyToolbarInputField(int windowX, int windowY)
    {
        return IsMouseOverToolbarField(windowX, windowY, SEARCH_INPUT_X_OFFSET, SEARCH_INPUT_WIDTH)
            || IsMouseOverToolbarField(windowX, windowY, LEVEL_MINIMUM_X_OFFSET, LEVEL_INPUT_WIDTH)
            || IsMouseOverToolbarField(windowX, windowY, LEVEL_MAXIMUM_X_OFFSET, LEVEL_INPUT_WIDTH);
    }

    // The input box only occupies the inset text area; a press on the frame around it should still focus it.
    void FocusInputOnFieldPress(CUITextInputBox& input, int windowX, int windowY, int fieldOffset, int fieldWidth)
    {
        if (IsPress(VK_LBUTTON) && !input.HaveFocus() && IsMouseOverToolbarField(windowX, windowY, fieldOffset, fieldWidth))
        {
            input.GiveFocus();
        }
    }

    void RenderTabLabel(int tabX, int tabY, int tabWidth, const wchar_t* label, int badgeCount, bool active)
    {
        g_pRenderText->SetTextColor(active ? 255 : 205, active ? 215 : 205, active ? 100 : 205, 255);
        const SIZE labelSize = g_pRenderText->MeasureText(label, static_cast<int>(wcslen(label)));
        const float textY = static_cast<float>(tabY + (TAB_HEIGHT - labelSize.cy) / 2);
        if (badgeCount <= 0)
        {
            g_pRenderText->RenderText(static_cast<float>(tabX), textY, label, static_cast<float>(tabWidth), 0, RT3_SORT_CENTER);
            return;
        }

        // Label and pending-count badge are laid out as one centred run, so the count never overprints the name.
        wchar_t badge[16];
        mu_swprintf(badge, L"[%d]", badgeCount);
        const SIZE badgeSize = g_pRenderText->MeasureText(badge, static_cast<int>(wcslen(badge)));
        const float runX = static_cast<float>(tabX) + (tabWidth - (labelSize.cx + TAB_BADGE_GAP + badgeSize.cx)) / 2.f;
        g_pRenderText->RenderText(runX, textY, label, static_cast<float>(labelSize.cx + 2), 0, RT3_SORT_LEFT);
        g_pRenderText->SetTextColor(255, 190, 70, 255);
        g_pRenderText->RenderText(runX + labelSize.cx + TAB_BADGE_GAP, textY, badge,
            static_cast<float>(badgeSize.cx + 2), 0, RT3_SORT_LEFT);
    }

    // What the wallet cards show, read once from the Open response so the 2D pass (counts, panels) and the 3D
    // pass (icons) lay out from the same data. A jewel bank later feeds the same view from a different source.
    struct WalletEntry
    {
        int itemType = 0;
        int itemLevel = 0;
        uint64_t amount = 0;
    };

    struct WalletView
    {
        std::vector<WalletEntry> jewels;
        std::vector<WalletEntry> fruits;
        bool hasZen = false;
        uint32_t zen = 0;
    };

    WalletView BuildWalletView(const AuctionHouse::AuctionOpenResponse& openResponse)
    {
        WalletView view;
        for (const auto& currency : openResponse.Currencies)
        {
            if (!currency.Enabled)
            {
                continue;
            }

            if (currency.CurrencyMode == AuctionCurrencyMode::Zen)
            {
                view.hasZen = true;
                view.zen = static_cast<uint32_t>(std::max<int64_t>(currency.Spendable.Scalar(), 0));
            }
            else if (currency.CurrencyMode == AuctionCurrencyMode::Fruits)
            {
                const auto& fruits = currency.Spendable.Fruits();
                const std::array<int64_t, 5> amounts = { fruits.Strength, fruits.Agility, fruits.Vitality, fruits.Energy,
                    fruits.Command };
                for (size_t fruit = 0; fruit < currency.FruitItemTypes.size(); ++fruit)
                {
                    if (currency.FruitItemTypes[fruit] != 0)
                    {
                        view.fruits.push_back({ currency.FruitItemTypes[fruit], currency.FruitItemLevels[fruit],
                            static_cast<uint64_t>(std::max<int64_t>(amounts[fruit], 0)) });
                    }
                }
            }
            else if (currency.LooseItemType != 0)
            {
                view.jewels.push_back({ currency.LooseItemType, 0,
                    static_cast<uint64_t>(std::max<int64_t>(currency.Spendable.Scalar(), 0)) });
            }
        }
        return view;
    }

    // Vertical placement of the wallet panels, window-relative. A panel with nothing to show is skipped and
    // the ones below move up.
    struct WalletLayout
    {
        int jewelsY = 0;
        int fruitsY = 0;
        int zenY = 0;
        int jewelsHeight = 0;
        int fruitsHeight = 0;
    };

    int WalletPanelHeight(size_t cellCount, int columns)
    {
        const int rows = AuctionHouse::WalletRowCount(cellCount, columns);
        return WALLET_PANEL_PAD_TOP + (rows - 1) * WALLET_ROW_PITCH + WALLET_ROW_CONTENT_HEIGHT + WALLET_PANEL_PAD_BOTTOM;
    }

    WalletLayout ComputeWalletLayout(const WalletView& view)
    {
        WalletLayout layout;
        int y = WALLET_Y;
        if (!view.jewels.empty())
        {
            layout.jewelsY = y;
            layout.jewelsHeight = WalletPanelHeight(view.jewels.size(), WALLET_JEWEL_COLUMNS);
            y += layout.jewelsHeight + WALLET_PANEL_GAP;
        }
        if (!view.fruits.empty())
        {
            layout.fruitsY = y;
            layout.fruitsHeight = WalletPanelHeight(view.fruits.size(), WALLET_FRUIT_COLUMNS);
            y += layout.fruitsHeight + WALLET_PANEL_GAP;
        }
        layout.zenY = y;
        return layout;
    }

    struct WalletCellRect
    {
        int x = 0;
        int y = 0;
        int width = 0;
    };

    WalletCellRect GetWalletCell(int panelY, size_t index, size_t cellCount, int columns)
    {
        const AuctionHouse::WalletCellSlot slot = AuctionHouse::LayoutWalletCell(index, cellCount, columns, WALLET_WIDTH);
        return { WALLET_X + slot.x, panelY + WALLET_PANEL_PAD_TOP + slot.row * WALLET_ROW_PITCH, slot.width };
    }

    // The counts under the icons, in one font and colour. A count that does not fit its card as exact digits
    // drops to the most precise compact form that does.
    void RenderWalletCounts(int windowX, int windowY, int panelY, const std::vector<WalletEntry>& entries, int columns)
    {
        const auto measureWidth = [](const std::wstring& text)
        {
            return static_cast<int>(g_pRenderText->MeasureText(text.c_str(), static_cast<int>(text.size())).cx);
        };

        for (size_t index = 0; index < entries.size(); ++index)
        {
            const WalletCellRect cell = GetWalletCell(panelY, index, entries.size(), columns);
            const std::wstring text = AuctionHouse::FormatWalletCountToFit(
                entries[index].amount, cell.width - 2 * WALLET_NUMBER_SIDE_PAD, measureWidth);
            g_pRenderText->RenderText(static_cast<float>(windowX + cell.x),
                static_cast<float>(windowY + cell.y + WALLET_ICON_ZONE + WALLET_NUMBER_GAP), text.c_str(),
                static_cast<float>(cell.width), 0, RT3_SORT_CENTER);
        }
    }

    // The icons, each fitted into the same square (iconSize) centred over its card in the icon band.
    void RenderWalletIcons(int windowX, int windowY, int panelY, const std::vector<WalletEntry>& entries, int columns,
        int iconSize)
    {
        for (size_t index = 0; index < entries.size(); ++index)
        {
            const WalletCellRect cell = GetWalletCell(panelY, index, entries.size(), columns);
            RenderItem3DFittedToSlot(static_cast<float>(windowX + cell.x + (cell.width - iconSize) / 2),
                static_cast<float>(windowY + cell.y + (WALLET_ICON_ZONE - iconSize) / 2), static_cast<float>(iconSize),
                static_cast<float>(iconSize), entries[index].itemType, entries[index].itemLevel, 0, 0,
                WALLET_ICON_FILL, false);
        }
    }

    // Sort indicator drawn as a tiny solid triangle instead of a "^"/"v" character, which is thin and sits
    // at an arbitrary height in the text box. Direction: 1 ascending (points up), -1 descending, 0 unsorted.
    constexpr int SORT_ARROW_WIDTH = 5;
    constexpr int SORT_ARROW_ROWS = 3;
    constexpr int SORT_ARROW_GAP = 4;
    constexpr int SORT_ARROW_Y_OFFSET = 3;
    constexpr unsigned int SORT_ARROW_COLOR = 0xFFFFFFFFu;

    void RenderSortableHeader(int columnX, int columnWidth, int textY, const wchar_t* text, int direction)
    {
        if (direction == 0)
        {
            g_pRenderText->RenderText(static_cast<float>(columnX), static_cast<float>(textY), text,
                static_cast<float>(columnWidth), 0, RT3_SORT_CENTER);
            return;
        }

        // Label and arrow are centred together as one run.
        const SIZE textSize = g_pRenderText->MeasureText(text, static_cast<int>(wcslen(text)));
        const int runWidth = textSize.cx + SORT_ARROW_GAP + SORT_ARROW_WIDTH;
        const int startX = columnX + std::max(1, (columnWidth - runWidth) / 2);
        g_pRenderText->RenderText(static_cast<float>(startX), static_cast<float>(textY), text,
            static_cast<float>(textSize.cx + 2), 0, RT3_SORT_LEFT);

        const int arrowX = startX + textSize.cx + SORT_ARROW_GAP;
        EnableAlphaTest();
        for (int row = 0; row < SORT_ARROW_ROWS; ++row)
        {
            const int width = direction > 0 ? 1 + 2 * row : SORT_ARROW_WIDTH - 2 * row;
            RenderColorQuadARGB(arrowX + (SORT_ARROW_WIDTH - width) / 2, textY + SORT_ARROW_Y_OFFSET + row, width, 1,
                SORT_ARROW_COLOR);
        }
        EndRenderColor();
    }

    // The rounded container that holds a listing table's header and its row cards.
    void RenderTableContainer(int x, int y, int width)
    {
        UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(TABLE_CONTAINER_HEIGHT), TABLE_CONTAINER_TINT);
    }

    // One row card, as tall as an item slot and one pixel inside its row. Gold ring and a faint wash when pointed at.
    void RenderRowCard(int x, int rowY, int width, bool hovered)
    {
        const UI::NineSlice::StateTints& tints = AuctionRowSkin().tints;
        const int y = rowY + BROWSE_ICON_CELL_OFFSET;
        UI::NineSlice::Render(AuctionRowSkin().texture, static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(BROWSE_ICON_CELL_SIZE), hovered ? tints.hover : tints.normal);
        if (hovered)
        {
            EnableAlphaTest();
            RenderColorQuadARGB(x + 2, y + 2, width - 4, BROWSE_ICON_CELL_SIZE - 4, ROW_HOVER_WASH);
            EndRenderColor();
        }
    }

    // The item that stands for a price component: a jewel or a stat fruit, as the Open response names them.
    // Zen has no item, and a component whose currency the server has not described yet has none either.
    struct PriceIcon
    {
        int itemType = 0;
        int itemLevel = 0;
    };

    std::optional<PriceIcon> ResolvePriceIcon(const AuctionHouse::AuctionOpenResponse& openResponse, AuctionHouse::PriceKind kind)
    {
        using AuctionHouse::PriceKind;
        if (kind == PriceKind::Zen)
        {
            return std::nullopt;
        }

        if (kind <= PriceKind::Harmony)
        {
            const auto* descriptor = FindJewelCurrencyDescriptor(openResponse,
                static_cast<AuctionCurrencyMode>(static_cast<int>(kind)));
            return descriptor == nullptr ? std::nullopt : std::optional<PriceIcon>(PriceIcon{ descriptor->LooseItemType, 0 });
        }

        const size_t fruit = static_cast<size_t>(kind) - static_cast<size_t>(PriceKind::StrengthFruit);
        for (const auto& currency : openResponse.Currencies)
        {
            if (currency.Enabled && currency.CurrencyMode == AuctionCurrencyMode::Fruits
                && currency.FruitItemTypes[fruit] != 0)
            {
                return PriceIcon{ currency.FruitItemTypes[fruit], currency.FruitItemLevels[fruit] };
            }
        }
        return std::nullopt;
    }

    // One chip of a price cell, placed relative to the cell's left edge and the row's top edge.
    struct PriceChip
    {
        AuctionHouse::PriceKind kind = AuctionHouse::PriceKind::Zen;
        std::wstring text;
        std::optional<PriceIcon> icon;
        int x = 0;
        int textY = 0;
    };

    // Lays a price's components out as chips over at most two lines, each line centred in the cell. The amounts
    // start as exact digits and drop to compact forms (12.3k, 1.5M) only as far as it takes to fit; the 2D pass
    // (text) and the 3D pass (icons) both call this so they always agree. Needs the bold font set by the caller
    // for measuring; sets it itself to be safe.
    std::vector<PriceChip> LayoutPriceCell(const std::vector<AuctionHouse::PriceComponent>& components, int cellWidth,
        const AuctionHouse::AuctionOpenResponse* openResponse)
    {
        std::vector<PriceChip> chips(components.size());
        for (size_t i = 0; i < components.size(); ++i)
        {
            chips[i].kind = components[i].kind;
            if (openResponse != nullptr)
            {
                chips[i].icon = ResolvePriceIcon(*openResponse, components[i].kind);
            }
        }

        g_pRenderText->SetFont(g_hFontBold);
        const auto measureWidth = [](const std::wstring& text)
        {
            return static_cast<int>(g_pRenderText->MeasureText(text.c_str(), static_cast<int>(text.size())).cx);
        };

        const int lineWidth = cellWidth - 2 * PRICE_CELL_PAD;
        std::vector<int> widths(components.size());
        std::optional<std::vector<AuctionHouse::PriceChipPlacement>> placements;
        for (size_t precision = 0; precision < 3 && !placements; ++precision)
        {
            for (size_t i = 0; i < components.size(); ++i)
            {
                const auto candidates = AuctionHouse::WalletCountCandidates(
                    static_cast<uint64_t>(std::max<int64_t>(components[i].amount, 0)));
                chips[i].text = candidates[std::min(precision, candidates.size() - 1)];
                if (!chips[i].icon)
                {
                    chips[i].text += L" ";
                    chips[i].text += PriceKindName(chips[i].kind);
                }
                widths[i] = (chips[i].icon ? PRICE_ICON_SIZE + PRICE_ICON_GAP : 0) + measureWidth(chips[i].text);
            }
            placements = AuctionHouse::PackPriceChips(widths, lineWidth, PRICE_CHIP_GAP, PRICE_MAX_LINES);
        }
        if (!placements)
        {
            // Even the most compact text needs more than two lines; let it run to as many as it takes.
            placements = AuctionHouse::PackPriceChips(widths, lineWidth, PRICE_CHIP_GAP, static_cast<int>(widths.size()) + 1);
        }
        if (!placements)
        {
            return {};
        }

        int lines = 0;
        for (const auto& placement : *placements)
        {
            lines = std::max(lines, placement.line + 1);
        }
        for (size_t i = 0; i < chips.size(); ++i)
        {
            const AuctionHouse::PriceChipPlacement& placement = (*placements)[i];
            chips[i].x = PRICE_CELL_PAD + placement.x;
            chips[i].textY = BROWSE_ROW_TEXT_Y_OFFSET + (placement.line * 2 - (lines - 1)) * (PRICE_LINE_HEIGHT / 2);
        }
        return chips;
    }

    // The amounts of a price cell, in the price colour. An empty price (no buyout set) is a quiet dash.
    void RenderPriceCellText(int cellX, int cellWidth, int rowY, const std::vector<PriceChip>& chips)
    {
        if (chips.empty())
        {
            g_pRenderText->SetFont(g_hFont);
            g_pRenderText->SetTextColor(120, 126, 138, 255);
            g_pRenderText->RenderText(static_cast<float>(cellX), static_cast<float>(rowY + BROWSE_ROW_TEXT_Y_OFFSET),
                L"-", static_cast<float>(cellWidth), 0, RT3_SORT_CENTER);
            return;
        }

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(238, 224, 168, 255);
        for (const PriceChip& chip : chips)
        {
            const int textX = cellX + chip.x + (chip.icon ? PRICE_ICON_SIZE + PRICE_ICON_GAP : 0);
            g_pRenderText->RenderText(static_cast<float>(textX), static_cast<float>(rowY + chip.textY),
                chip.text.c_str(), 0.f, 0, RT3_SORT_LEFT);
        }
    }

    // A rounded card of the detail views.
    void RenderContainer(int x, int y, int width, int height)
    {
        UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(height), TABLE_CONTAINER_TINT);
    }

    // A small bold caption over a card's content.
    void RenderCardCaption(const wchar_t* text, int x, int y, int width, int sort)
    {
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(218, 186, 104, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(y), text, static_cast<float>(width), 0, sort);
    }

    // One line of text in the given font and colour.
    void RenderCardLine(const std::wstring& text, int x, int y, int width, bool bold, int r, int g, int b, int sort = RT3_SORT_LEFT)
    {
        g_pRenderText->SetFont(bold ? g_hFontBold : g_hFont);
        g_pRenderText->SetTextColor(r, g, b, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(y), text.c_str(), static_cast<float>(width), 0, sort);
    }

    // The icons of a price cell's chips (the 3D pass; the amounts are drawn by RenderPriceCellText).
    void RenderPriceChipIcons(int cellX, int rowY, const std::vector<PriceChip>& chips)
    {
        for (const PriceChip& chip : chips)
        {
            if (chip.icon)
            {
                RenderItem3DFittedToSlot(static_cast<float>(cellX + chip.x),
                    static_cast<float>(rowY + chip.textY + PRICE_ICON_Y_OFFSET), static_cast<float>(PRICE_ICON_SIZE),
                    static_cast<float>(PRICE_ICON_SIZE), chip.icon->itemType, chip.icon->itemLevel, 0, 0, PRICE_ICON_FILL,
                    false);
            }
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
      m_bOpenedInventoryForSell(false), m_bSellDocked(false), m_BrowsePos{ 0, 0 },
      m_SellDockedForWidth(0), m_SellDockedForHeight(0), m_bHasSellItem(false), m_SellInventorySlot(0),
      m_SellItemSnapshot{}, m_bPointingSellItem(false),
      m_SelectedCurrency(static_cast<AuctionCurrencyMode>(ALL_CURRENCIES)), m_SelectedCategoryIndex(0), m_CurrentPage(1), m_SelectedSort(AuctionSort::EndingSoonest),
      m_bBrowseRequestPending(false), m_PendingBrowseRequestId(0), m_bHasListingResponse(false),
      m_SelectedStatusIndex(0), m_bMyListingsRequestPending(false), m_PendingMyListingsRequestId(0),
      m_SelectedCollectionKindIndex(0), m_bMailboxRequestPending(false), m_PendingMailboxRequestId(0),
      m_bHasMailboxResponse(false), m_PendingMailboxCount(0), m_bHasSelectedCollection(false),
      m_BrowseScrollOffset(0), m_iPointedRow(-1), m_iPointedItemRow(-1),
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

    // Index zero is the wire's 0xFF all-currencies sentinel. Following entries map to the nine contiguous
    // AuctionCurrencyMode values, so item-name search can return every matching listing rather than only Zen.
    m_CurrencyLabels[0] = L"All Currencies";
    m_CurrencyLabels[1] = I18N::Game::Zen;
    m_CurrencyLabels[2] = I18N::Game::JewelOfChaos;
    m_CurrencyLabels[3] = I18N::Game::JewelOfBless;
    m_CurrencyLabels[4] = I18N::Game::JewelOfSoul;
    m_CurrencyLabels[5] = I18N::Game::JewelOfLife;
    m_CurrencyLabels[6] = I18N::Game::JewelOfCreation;
    m_CurrencyLabels[7] = I18N::Game::JewelOfGuardian;
    m_CurrencyLabels[8] = I18N::Game::JewelOfHarmony;
    m_CurrencyLabels[9] = I18N::Game::Fruits;
    m_CurrencyCombo.Setup(m_Pos.x + TOOLBAR_X + CURRENCY_COMBO_X_OFFSET, m_Pos.y + TOOLBAR_Y, CURRENCY_COMBO_WIDTH, CURRENCY_COMBO_ITEM_HEIGHT,
        m_CurrencyLabels, 10, 0);
    m_CurrencyCombo.SetFieldSkin(&AuctionFieldSkin());

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
    m_StatusCombo.SetFieldSkin(&AuctionFieldSkin());

    m_CollectionKindLabels[0] = I18N::Game::AuctionAllCollections;
    m_CollectionKindLabels[1] = I18N::Game::AuctionPurchasedItem;
    m_CollectionKindLabels[2] = I18N::Game::AuctionReturnedItem;
    m_CollectionKindLabels[3] = I18N::Game::AuctionSaleProceeds;
    m_CollectionKindLabels[4] = I18N::Game::AuctionOutbidRefund;
    m_CollectionKindLabels[5] = I18N::Game::AuctionTenderChange;
    m_CollectionKindLabels[6] = I18N::Game::AuctionAdminRefund;
    m_CollectionKindCombo.Setup(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, CATEGORY_COMBO_WIDTH,
        CURRENCY_COMBO_ITEM_HEIGHT, m_CollectionKindLabels, 7, m_SelectedCollectionKindIndex);
    m_CollectionKindCombo.SetFieldSkin(&AuctionFieldSkin());

    // The inputs sit inside a skinned field drawn behind them, so they paint no background of their own and
    // are inset to leave the field's frame clear.
    m_SearchInput.Init(g_hWnd, SEARCH_INPUT_WIDTH - 2 * TOOLBAR_FIELD_TEXT_INSET, SEARCH_INPUT_HEIGHT, 32, false);
    m_SearchInput.SetPosition(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET + TOOLBAR_FIELD_TEXT_INSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_SearchInput.SetTextColor(255, 255, 230, 210);
    m_SearchInput.SetBackColor(0, 0, 0, 0);
    m_SearchInput.SetSelectBackColor(255, 95, 68, 24);
    m_SearchInput.SetFont(g_hFont);
    m_SearchInput.SetParentUIID(SEASON3B::INTERFACE_AUCTION_HOUSE);
    m_SearchInput.SetState(UISTATE_NORMAL);

    for (auto* levelInput : { &m_MinimumLevelInput, &m_MaximumLevelInput })
    {
        levelInput->Init(g_hWnd, LEVEL_INPUT_WIDTH - 2 * LEVEL_FIELD_TEXT_INSET, SEARCH_INPUT_HEIGHT, 3, false);
        levelInput->SetTextColor(255, 255, 230, 210);
        levelInput->SetBackColor(0, 0, 0, 0);
        levelInput->SetSelectBackColor(255, 95, 68, 24);
        levelInput->SetFont(g_hFont);
        levelInput->SetParentUIID(SEASON3B::INTERFACE_AUCTION_HOUSE);
        levelInput->SetOption(UIOPTION_NUMBERONLY);
        levelInput->SetState(UISTATE_NORMAL);
    }
    m_MinimumLevelInput.SetPosition(m_Pos.x + TOOLBAR_X + LEVEL_MINIMUM_X_OFFSET + LEVEL_FIELD_TEXT_INSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_MaximumLevelInput.SetPosition(m_Pos.x + TOOLBAR_X + LEVEL_MAXIMUM_X_OFFSET + LEVEL_FIELD_TEXT_INSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);

    for (size_t i = 0; i < std::size(m_CollectionClaimInputs); ++i)
    {
        auto& input = m_CollectionClaimInputs[i];
        input.Init(g_hWnd, CLAIM_INPUT_WIDTH, CLAIM_INPUT_HEIGHT, 10, false);
        input.SetPosition(m_Pos.x + CLAIM_INPUT_X + static_cast<int>(i) * CLAIM_INPUT_COLUMN_WIDTH, m_Pos.y + CLAIM_INPUT_Y);
        input.SetTextColor(255, 255, 230, 210);
        input.SetBackColor(0, 0, 0, 0);
        input.SetSelectBackColor(255, 95, 68, 24);
        input.SetFont(g_hFont);
        input.SetParentUIID(SEASON3B::INTERFACE_AUCTION_HOUSE);
        input.SetOption(UIOPTION_NUMBERONLY);
        input.SetState(UISTATE_HIDE);
    }

    InitPageButton(&m_BtnPrevPage, m_Pos.x + PageClusterX(m_iCurrentTab), m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Previous);
    InitPageButton(&m_BtnNextPage, m_Pos.x + PageNextButtonX(m_iCurrentTab), m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Next);
    InitPageButton(&m_BtnBack, m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, I18N::Game::Back);
    InitPageButton(&m_BtnBid, m_Pos.x + PAGE_BTN_MARGIN_X, m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Bid);
    InitPageButton(&m_BtnBuyout, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH, m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Buyout);
    InitPageButton(&m_BtnCancelListing, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::Cancel);
    InitPageButton(&m_BtnCollect, m_Pos.x + WINDOW_WIDTH - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, I18N::Game::AuctionCollect);
    InitPageButton(&m_BtnSearch, m_Pos.x + TOOLBAR_X + SEARCH_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, I18N::Game::AuctionSearch);
    InitPageButton(&m_BtnReset, m_Pos.x + TOOLBAR_X + RESET_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, L"Reset");
    m_BtnSearch.ChangeButtonInfo(m_Pos.x + TOOLBAR_X + SEARCH_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, SEARCH_BUTTON_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnReset.ChangeButtonInfo(m_Pos.x + TOOLBAR_X + RESET_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, RESET_BUTTON_WIDTH, PAGE_BTN_HEIGHT);
    m_BrowseScrollBar.Create(m_Pos.x + WINDOW_WIDTH - BROWSE_SCROLLBAR_RIGHT_MARGIN,
        m_Pos.y + BROWSE_BODY_Y + BROWSE_SCROLLBAR_TOP_INSET, BROWSE_SCROLLBAR_HEIGHT);
    m_BrowseScrollBar.Show(false);
    m_SellPanel.Create(m_Pos.x, m_Pos.y, BUY_NOW_SUPPORTED);

    Show(false);

    return true;
}

void SEASON3B::CNewUIAuctionWindow::Release()
{
    m_SearchInput.SetState(UISTATE_HIDE);
    m_MinimumLevelInput.SetState(UISTATE_HIDE);
    m_MaximumLevelInput.SetState(UISTATE_HIDE);
    SetCollectionClaimInputsVisible(false);
    if (m_SearchInput.HaveFocus() || m_MinimumLevelInput.HaveFocus() || m_MaximumLevelInput.HaveFocus())
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
    pButton->SetNineSliceSkin(&AuctionButtonSkin());
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
    m_CurrencyCombo.SetPos(m_Pos.x + TOOLBAR_X + CURRENCY_COMBO_X_OFFSET, m_Pos.y + TOOLBAR_Y);
    m_CategoryCombo.SetPos(m_Pos.x + TOOLBAR_X + CATEGORY_COMBO_X_OFFSET, m_Pos.y + TOOLBAR_Y);
    m_StatusCombo.SetPos(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y);
    m_CollectionKindCombo.SetPos(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y);
    for (size_t i = 0; i < std::size(m_CollectionClaimInputs); ++i)
    {
        m_CollectionClaimInputs[i].SetPosition(
            m_Pos.x + CLAIM_INPUT_X + static_cast<int>(i) * CLAIM_INPUT_COLUMN_WIDTH, m_Pos.y + CLAIM_INPUT_Y);
    }
    m_SearchInput.SetPosition(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET + TOOLBAR_FIELD_TEXT_INSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_MinimumLevelInput.SetPosition(m_Pos.x + TOOLBAR_X + LEVEL_MINIMUM_X_OFFSET + LEVEL_FIELD_TEXT_INSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_MaximumLevelInput.SetPosition(m_Pos.x + TOOLBAR_X + LEVEL_MAXIMUM_X_OFFSET + LEVEL_FIELD_TEXT_INSET, m_Pos.y + SEARCH_INPUT_Y_OFFSET);
    m_BtnSearch.ChangeButtonInfo(m_Pos.x + TOOLBAR_X + SEARCH_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y, SEARCH_BUTTON_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnReset.ChangeButtonInfo(m_Pos.x + TOOLBAR_X + RESET_BUTTON_X_OFFSET, m_Pos.y + TOOLBAR_Y,
        RESET_BUTTON_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnPrevPage.ChangeButtonInfo(m_Pos.x + PageClusterX(m_iCurrentTab), m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnNextPage.ChangeButtonInfo(m_Pos.x + PageNextButtonX(m_iCurrentTab), m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnBack.ChangeButtonInfo(m_Pos.x + TOOLBAR_X, m_Pos.y + TOOLBAR_Y, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnBid.ChangeButtonInfo(m_Pos.x + PAGE_BTN_MARGIN_X, m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnBuyout.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH, m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnCancelListing.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BtnCollect.ChangeButtonInfo(m_Pos.x + windowWidth - PAGE_BTN_MARGIN_X - PAGE_BTN_WIDTH,
        m_Pos.y + PAGE_BTN_Y_OFFSET, PAGE_BTN_WIDTH, PAGE_BTN_HEIGHT);
    m_BrowseScrollBar.SetPos(m_Pos.x + windowWidth - BROWSE_SCROLLBAR_RIGHT_MARGIN,
        m_Pos.y + BROWSE_BODY_Y + BROWSE_SCROLLBAR_TOP_INSET);
    m_BrowseScrollBar.UpdateScrolling();
    m_SellPanel.SetPos(m_Pos.x, m_Pos.y);
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
    m_SellPanel.ClearForm();
    m_SellPanel.SetActive(false);
    m_SearchInput.SetText(L"");
    m_SearchInput.SetState(UISTATE_NORMAL);
    m_MinimumLevelInput.SetText(L"");
    m_MaximumLevelInput.SetText(L"");
    m_MinimumLevelInput.SetState(UISTATE_NORMAL);
    m_MaximumLevelInput.SetState(UISTATE_NORMAL);
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
    BYTE minimumLevel = ReadAuctionLevelInput(m_MinimumLevelInput, 0);
    BYTE maximumLevel = ReadAuctionLevelInput(m_MaximumLevelInput, 255);
    if (minimumLevel > maximumLevel)
    {
        std::swap(minimumLevel, maximumLevel);
    }
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
        minimumLevel,
        maximumLevel,
        0, // option flags: none required
        0, 0, 0, 0, 0, 0, // minimum price, every component: no floor
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, // maximum price, every component: no ceiling
        0, // max remaining hours: no limit
        static_cast<BYTE>(searchUtf8.size()),
        searchUtf8.empty() ? nullptr : reinterpret_cast<const BYTE*>(searchUtf8.data()),
        static_cast<uint32_t>(searchUtf8.size()),
        0xFF); // sub-category: every one (Task 9 sends the picked leaf)
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

void SEASON3B::CNewUIAuctionWindow::SendCreateRequest(const AuctionHouse::SellDraft& draft, const std::wstring& note)
{
    if (!m_bHasSellItem || m_bOperationRequestPending)
    {
        return;
    }

    // The create packet has one currency mode with a scalar or fruit-basket price, so only an auction maps onto it
    // today. The panel already refuses to submit anything else; this is the backstop.
    const auto fields = AuctionHouse::ToCreateFields(draft);
    if (!fields)
    {
        m_SellPanel.ShowResult(false, L"Buy Now listings are not available on this server yet.");
        return;
    }

    std::string noteUtf8;
    mu_wchar_to_utf8(note.c_str(), noteUtf8);
    noteUtf8 = AuctionHouse::ClampUtf8ToByteCapacity(noteUtf8, 80);

    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_bOperationRequestPending = true;
    m_bHasOperationResult = false;
    m_SellPanel.SetBusy(true);

    const auto& starting = fields->starting.Fruits();
    const auto& buyout = fields->buyout.Fruits();
    SocketClient->ToGameServer()->SendAuctionCreateRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()), m_SellInventorySlot,
        fields->currencyMode,
        static_cast<uint32_t>(fields->starting.Scalar()),
        static_cast<uint32_t>(starting.Strength), static_cast<uint32_t>(starting.Agility),
        static_cast<uint32_t>(starting.Vitality), static_cast<uint32_t>(starting.Energy),
        static_cast<uint32_t>(starting.Command),
        static_cast<uint32_t>(fields->buyout.Scalar()),
        static_cast<uint32_t>(buyout.Strength), static_cast<uint32_t>(buyout.Agility),
        static_cast<uint32_t>(buyout.Vitality), static_cast<uint32_t>(buyout.Energy),
        static_cast<uint32_t>(buyout.Command),
        draft.durationHours, static_cast<BYTE>(noteUtf8.size()),
        reinterpret_cast<const BYTE*>(noteUtf8.data()), static_cast<uint32_t>(noteUtf8.size()));
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

    if (response.OperationType == AuctionOperationType::Create)
    {
        m_SellPanel.SetBusy(false);
        const bool created = result == AuctionResult::Success;
        m_SellPanel.ShowResult(created, AuctionHouse::DescribeCreateResult(result));
        if (created)
        {
            // The item left the inventory and the fee left the wallet: forget the picked item and refresh the
            // wallet and Mailbox count from the server instead of adjusting them locally.
            m_bHasSellItem = false;
            m_bPointingSellItem = false;
            m_PendingOpenRequestId = AuctionHouse::NextAuctionRequestId();
            m_bOpenRequestPending = true;
            SocketClient->ToGameServer()->SendAuctionOpenRequest(m_PendingOpenRequestId);
        }
        return;
    }

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
        m_LastOperationMessage = AuctionHouse::DescribeOperationResult(response.OperationType, result);
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
    m_LastOperationMessage = AuctionHouse::DescribeOperationResult(response.OperationType, result);
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

    const int iconX = m_Pos.x + RowSlotX(m_iCurrentTab) + BROWSE_ICON_CELL_OFFSET + BROWSE_ICON_MARGIN;
    const int iconY = m_Pos.y + BROWSE_BODY_Y + row * BROWSE_ROW_HEIGHT + BROWSE_ICON_CELL_OFFSET + BROWSE_ICON_MARGIN;
    RenderItemInfo(iconX + BROWSE_ICON_SIZE / 2, iconY + BROWSE_ICON_SIZE / 2, m_RowItems[row], false, TOOLTIP_TYPE_AUCTION);
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

    const int iconSize = DETAIL_SLOT_SIZE - 2 * DETAIL_ICON_INSET;
    const int iconX = m_Pos.x + DETAIL_SLOT_X + DETAIL_ICON_INSET;
    const int iconY = m_Pos.y + DETAIL_SLOT_Y + DETAIL_ICON_INSET;
    RenderItemInfo(iconX + iconSize / 2, iconY + iconSize / 2, m_DetailItem, false, TOOLTIP_TYPE_AUCTION);
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
    m_SellPanel.SetServerSettings(response.ListingFeeBasisPoints, response.SuccessFeeBasisPoints, response.DurationMask,
        response.DefaultDurationHours);
}

void SEASON3B::CNewUIAuctionWindow::ClosingProcess()
{
    LeaveSellTab();
    m_SellPanel.SetActive(false);
    m_bDragging = false;
    m_SearchInput.SetState(UISTATE_HIDE);
    SetCollectionClaimInputsVisible(false);
    if (m_SearchInput.HaveFocus() || m_MinimumLevelInput.HaveFocus() || m_MaximumLevelInput.HaveFocus())
    {
        CUITextInputBox::ReleaseFocus();
    }
    SetRelatedWnd(g_hWnd);
}

void SEASON3B::CNewUIAuctionWindow::EnterSellTab()
{
    // Sell is the same size as Browse but needs the inventory beside it, so it leaves the centred dialog layout
    // for one anchored to the right edge like the inventory's, at the dialog scale so it does not grow, and sits
    // directly to the inventory's left.
    m_BrowsePos = m_Pos;
    m_bSellDocked = true;
    SetLayoutMode(UI::Scaling::LayoutMode::PanelDockRight);
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
    {
        m_bOpenedInventoryForSell = true;
        g_pNewUISystem->Show(SEASON3B::INTERFACE_INVENTORY);
    }
    DockSellWindow();

    // Items are chosen by pointing at them; nothing in the inventory can be picked up while Sell is open.
    g_pMyInventory->SetSelectionOnly(true);
}

void SEASON3B::CNewUIAuctionWindow::DockSellWindow()
{
    // Sell stays exactly where the other tabs are on screen. Only when that would put it over the inventory does it
    // slide left, by just the overlap. The inventory is drawn at the dock scale (a little larger than the dialog scale
    // on big screens), so all of this is worked out in screen pixels and converted into Sell's own units at the end.
    const UI::Scaling::Transform inventoryDock = UI::Scaling::DockRightTransform(WindowWidth, WindowHeight);
    const UI::Scaling::Transform dialog = UI::Scaling::PanelTransform(WindowWidth, WindowHeight);
    const UI::Scaling::Transform sellDock = UI::Scaling::PanelDockRightTransform(WindowWidth, WindowHeight);

    const POINT inventoryPos = g_pMyInventory->GetPos();
    const float inventoryLeft = UI::Scaling::PositionX(inventoryDock, static_cast<float>(inventoryPos.x));
    const float rightLimit = inventoryLeft - SELL_INVENTORY_GAP * sellDock.scaleX;
    const float sameSpotLeft = UI::Scaling::PositionX(dialog, static_cast<float>(m_BrowsePos.x));

    const int desiredLeft = static_cast<int>(std::lround(UI::Scaling::LogicalX(sellDock, sameSpotLeft)));
    const int limit = static_cast<int>(std::floor(UI::Scaling::LogicalX(sellDock, rightLimit)));
    const int screenLeft = static_cast<int>(std::ceil(UI::Scaling::LogicalX(sellDock, 0.0f)));

    m_Pos.x = AuctionHouse::SideBySideLeft(desiredLeft, WINDOW_WIDTH, limit, screenLeft);
    m_Pos.y = m_BrowsePos.y;
    m_SellDockedForWidth = WindowWidth;
    m_SellDockedForHeight = WindowHeight;
    RepositionChildren();
}

void SEASON3B::CNewUIAuctionWindow::LeaveSellTab()
{
    if (g_pMyInventory != nullptr)
    {
        g_pMyInventory->SetSelectionOnly(false);
    }

    if (m_bOpenedInventoryForSell && g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_INVENTORY);
    }

    if (m_bSellDocked)
    {
        m_Pos = m_BrowsePos;
        m_bSellDocked = false;
    }

    m_bOpenedInventoryForSell = false;
    m_bPointingSellItem = false;
}

void SEASON3B::CNewUIAuctionWindow::CaptureSellSelection()
{
    // The inventory is locked against picking items up while Sell is open, so a click on an item arrives as a
    // plain release over its square. Nothing moves: the item is only noted as the one to list.
    if (!IsRelease(VK_LBUTTON) || !g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INVENTORY))
    {
        return;
    }

    CNewUIInventoryCtrl* inventory = g_pMyInventory->GetInventoryCtrl();
    if (inventory == nullptr)
    {
        return;
    }

    ITEM* item = inventory->FindItemPointedSquareIndex();
    if (item == nullptr)
    {
        return;
    }

    const int slot = inventory->GetIndexByItem(item);
    if (IsPlayerInventorySlot(slot))
    {
        m_SellInventorySlot = static_cast<BYTE>(slot);
        m_SellItemSnapshot = *item;
        m_bHasSellItem = true;
    }
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
    const int closeX = m_Pos.x + CurrentWindowWidth() - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN;
    const int closeY = m_Pos.y + CLOSE_BTN_MARGIN;
    if (CheckMouseIn(closeX, closeY, CLOSE_BTN_WIDTH, CLOSE_BTN_HEIGHT) && IsRelease(VK_LBUTTON))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_AUCTION_HOUSE);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_iCurrentTab == TAB_SELL)
    {
        const bool handled = m_SellPanel.UpdateMouseEvent();
        // Keys go to whichever input has focus (the Sell tab has no other text field), or back to the game.
        const HWND focusedInput = m_SellPanel.FocusedInputHandle();
        SetRelatedWnd(focusedInput != nullptr ? focusedInput : g_hWnd);
        return handled;
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

    if ((m_SearchInput.HaveFocus() || m_MinimumLevelInput.HaveFocus() || m_MaximumLevelInput.HaveFocus()) && IsPress(VK_LBUTTON)
        && !IsMouseOverAnyToolbarInputField(m_Pos.x, m_Pos.y))
    {
        CUITextInputBox::ReleaseFocus();
        SetRelatedWnd(g_hWnd);
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
            m_MinimumLevelInput.DoAction();
            m_MaximumLevelInput.DoAction();
            FocusInputOnFieldPress(m_SearchInput, m_Pos.x, m_Pos.y, SEARCH_INPUT_X_OFFSET, SEARCH_INPUT_WIDTH);
            FocusInputOnFieldPress(m_MinimumLevelInput, m_Pos.x, m_Pos.y, LEVEL_MINIMUM_X_OFFSET, LEVEL_INPUT_WIDTH);
            FocusInputOnFieldPress(m_MaximumLevelInput, m_Pos.x, m_Pos.y, LEVEL_MAXIMUM_X_OFFSET, LEVEL_INPUT_WIDTH);
            if (m_SearchInput.HaveFocus())
            {
                SetRelatedWnd(m_SearchInput.GetHandle());
            }
            else if (m_MinimumLevelInput.HaveFocus())
            {
                SetRelatedWnd(m_MinimumLevelInput.GetHandle());
            }
            else if (m_MaximumLevelInput.HaveFocus())
            {
                SetRelatedWnd(m_MaximumLevelInput.GetHandle());
            }
            if (IsMouseOverAnyToolbarInputField(m_Pos.x, m_Pos.y))
            {
                return true;
            }

            if (m_BtnSearch.UpdateMouseEvent() == true)
            {
                SubmitSearch();
                return true;
            }
            if (m_BtnReset.UpdateMouseEvent() == true)
            {
                m_SearchInput.SetText(L"");
                m_MinimumLevelInput.SetText(L"");
                m_MaximumLevelInput.SetText(L"");
                m_SelectedCurrency = static_cast<AuctionCurrencyMode>(ALL_CURRENCIES);
                m_SelectedCategoryIndex = 0;
                m_SelectedSort = AuctionSort::EndingSoonest;
                m_CategoryCombo.SetSelectedIndex(m_SelectedCategoryIndex);
                m_CurrentPage = 1;
                SendBrowseRequest();
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }

        if (m_BrowseScrollBar.IsVisible())
        {
            m_BrowseScrollBar.UpdateMouseEvent();
        }

        if (m_BrowseScrollBar.IsVisible()
            && CheckMouseIn(m_Pos.x + GetTableLayout(m_iCurrentTab == TAB_BROWSE).x, m_Pos.y + BROWSE_BODY_Y,
            GetTableLayout(m_iCurrentTab == TAB_BROWSE).width, BROWSE_TABLE_HEIGHT) && MouseWheel != 0)
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

        const TableLayout browseTable = GetTableLayout(true);
        if (m_iCurrentTab == TAB_BROWSE && IsRelease(VK_LBUTTON)
            && CheckMouseIn(m_Pos.x + browseTable.x, m_Pos.y + BROWSE_HEADER_Y,
                browseTable.width, BROWSE_HEADER_HEIGHT))
        {
            const int localX = MouseX - (m_Pos.x + browseTable.innerX());
            AuctionSort requestedSort = m_SelectedSort;
            if (localX >= browseTable.timeX() && localX < browseTable.timeX() + TABLE_TIME_COLUMN_WIDTH)
            {
                requestedSort = AuctionHouse::NextBrowseSort(AuctionHouse::AuctionBrowseSortColumn::TimeLeft, m_SelectedSort);
            }
            else if (!IsAllCurrencies(m_SelectedCurrency) && m_SelectedCurrency != AuctionCurrencyMode::Fruits
                && localX >= browseTable.priceX() && localX < browseTable.priceX() + TABLE_PRICE_COLUMN_WIDTH)
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
        if (m_iCurrentTab == TAB_SELL && m_SellPanel.HasInputFocus())
        {
            if (SEASON3B::IsPress(VK_RETURN) == true)
            {
                m_SellPanel.RequestCreate();
                m_SellPanel.ReleaseInputFocus();
                SetRelatedWnd(g_hWnd);
                return false;
            }
            if (SEASON3B::IsPress(VK_ESCAPE) == true)
            {
                m_SellPanel.ReleaseInputFocus();
                SetRelatedWnd(g_hWnd);
                return false;
            }
            return true;
        }

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

        if (m_SearchInput.HaveFocus() || m_MinimumLevelInput.HaveFocus() || m_MaximumLevelInput.HaveFocus())
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
        // The compact tab row deliberately leaves clear space between the last tab and the close control, so
        // hit-testing follows the same fixed geometry that Render() uses.
        int selected = RADIOGROUPEVENT_NONE;
        if (IsRelease(VK_LBUTTON))
        {
            int tabX[TAB_COUNT];
            int tabWidths[TAB_COUNT];
            ComputeTabLayout(tabX, tabWidths);
            for (int tab = 0; tab < TAB_COUNT; ++tab)
            {
                if (CheckMouseIn(tabX[tab], m_Pos.y + TAB_REGION_Y, tabWidths[tab], TAB_HEIGHT))
                {
                    selected = tab;
                    break;
                }
            }
        }
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
            m_SellPanel.SetActive(m_iCurrentTab == TAB_SELL);
            m_SearchInput.SetState(m_iCurrentTab == TAB_BROWSE ? UISTATE_NORMAL : UISTATE_HIDE);
            m_MinimumLevelInput.SetState(m_iCurrentTab == TAB_BROWSE ? UISTATE_NORMAL : UISTATE_HIDE);
            m_MaximumLevelInput.SetState(m_iCurrentTab == TAB_BROWSE ? UISTATE_NORMAL : UISTATE_HIDE);
            if (m_iCurrentTab != TAB_BROWSE && (m_SearchInput.HaveFocus() || m_MinimumLevelInput.HaveFocus() || m_MaximumLevelInput.HaveFocus()))
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
            // Combo index 0 is the "All Currencies" sentinel; indices 1-9 map directly onto
            // AuctionCurrencyMode (Zen=0 .. Fruits=8), one past the combo index.
            const int selected = m_CurrencyCombo.GetSelectedIndex();
            m_SelectedCurrency = selected == 0
                ? static_cast<AuctionCurrencyMode>(ALL_CURRENCIES)
                : static_cast<AuctionCurrencyMode>(selected - 1);
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
        m_iPointedItemRow = -1;
        m_bPointingDetailItem = false;
        m_bPointingSellItem = false;
        if (m_iCurrentTab == TAB_SELL)
        {
            if (m_bSellDocked && !m_bDragging && (WindowWidth != m_SellDockedForWidth || WindowHeight != m_SellDockedForHeight))
            {
                DockSellWindow();
            }
            CaptureSellSelection();
            ValidateSellSelection();
            m_SellPanel.SetItemSelected(m_bHasSellItem);
            m_SellPanel.SetBusy(m_bOperationRequestPending);
            m_bPointingSellItem = m_SellPanel.IsMouseOverItemIcon();

            AuctionHouse::SellDraft draft;
            std::wstring note;
            if (m_SellPanel.TakeCreateRequest(draft, note))
            {
                SendCreateRequest(draft, note);
            }
        }
        if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS || m_iCurrentTab == TAB_MAILBOX) && m_bShowingDetail)
        {
            m_bPointingDetailItem = m_DetailItem != nullptr
                && CheckMouseIn(m_Pos.x + DETAIL_SLOT_X, m_Pos.y + DETAIL_SLOT_Y, DETAIL_SLOT_SIZE, DETAIL_SLOT_SIZE);
        }
        else if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS) && m_bHasListingResponse)
        {
            const auto remainingRows = m_ListingResponse.Listings.size() - std::min(m_BrowseScrollOffset, m_ListingResponse.Listings.size());
            const auto rowCount = std::min(MaxBrowseRows, remainingRows);
            const TableLayout table = GetTableLayout(m_iCurrentTab == TAB_BROWSE);
            for (size_t row = 0; row < rowCount; ++row)
            {
                if (CheckMouseIn(m_Pos.x + table.x, m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT,
                    table.width, BROWSE_ROW_HEIGHT))
                {
                    m_iPointedRow = static_cast<int>(row);
                    if (CheckMouseIn(m_Pos.x + table.innerX() + BROWSE_ICON_CELL_OFFSET,
                        m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT + BROWSE_ICON_CELL_OFFSET,
                        BROWSE_ICON_CELL_SIZE, BROWSE_ICON_CELL_SIZE))
                    {
                        m_iPointedItemRow = static_cast<int>(row);
                    }
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

        // An open dropdown owns the mouse: the rows beneath its list are neither hovered nor given a tooltip.
        if (m_CurrencyCombo.IsOpen() || m_StatusCombo.IsOpen() || m_CollectionKindCombo.IsOpen())
        {
            m_iPointedRow = -1;
            m_iPointedItemRow = -1;
        }
    }

    return true;
}

bool SEASON3B::CNewUIAuctionWindow::Render()
{
    EnableAlphaTest();

    RenderFrame();

    // Tabs share the buttons' nine-slice chrome: one fixed width each, gold when active and brightened steel
    // when hovered. RenderTabLabel lays out a pending-Mailbox count with its label so they never overprint.
    {
        const wchar_t* const tabLabelPtrs[TAB_COUNT] = {
            I18N::Game::Browse, I18N::Game::Sell, I18N::Game::MyListings, I18N::Game::Mailbox,
        };
        int tabX[TAB_COUNT];
        int tabWidths[TAB_COUNT];
        ComputeTabLayout(tabX, tabWidths);
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        const int tabY = m_Pos.y + TAB_REGION_Y;
        for (int tab = 0; tab < TAB_COUNT; ++tab)
        {
            const bool active = tab == m_iCurrentTab;
            const bool hovered = !active && CheckMouseIn(tabX[tab], tabY, tabWidths[tab], TAB_HEIGHT);
            UI::NineSlice::Render(AuctionButtonSkin().texture, static_cast<float>(tabX[tab]), static_cast<float>(tabY),
                static_cast<float>(tabWidths[tab]), static_cast<float>(TAB_HEIGHT),
                active ? TAB_TINT_ACTIVE : (hovered ? TAB_TINT_HOVER : TAB_TINT_IDLE));
            RenderTabLabel(tabX[tab], tabY, tabWidths[tab], tabLabelPtrs[tab],
                tab == TAB_MAILBOX ? static_cast<int>(m_PendingMailboxCount) : 0, active);
        }
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
            // Every toolbar control shares the nine-slice chrome: recessed fields behind the text inputs (a gold
            // ring while one has focus), raised buttons, and a field-skinned currency combo. The inputs paint no
            // background of their own and own only the keyboard behaviour.
            const auto renderToolbarField = [this](int xOffset, int width, bool focused)
            {
                const UI::NineSlice::StateTints& tints = AuctionFieldSkin().tints;
                UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(m_Pos.x + TOOLBAR_X + xOffset),
                    static_cast<float>(m_Pos.y + TOOLBAR_Y), static_cast<float>(width),
                    static_cast<float>(TOOLBAR_FIELD_HEIGHT), focused ? tints.pressed : tints.normal);
            };
            renderToolbarField(SEARCH_INPUT_X_OFFSET, SEARCH_INPUT_WIDTH, m_SearchInput.HaveFocus());
            renderToolbarField(LEVEL_MINIMUM_X_OFFSET, LEVEL_INPUT_WIDTH, m_MinimumLevelInput.HaveFocus());
            renderToolbarField(LEVEL_MAXIMUM_X_OFFSET, LEVEL_INPUT_WIDTH, m_MaximumLevelInput.HaveFocus());
            m_SearchInput.Render();
            m_MinimumLevelInput.Render();
            m_MaximumLevelInput.Render();
            wchar_t searchText[33]{};
            m_SearchInput.GetText(searchText, static_cast<int>(std::size(searchText)));
            if (searchText[0] == L'\0' && !m_SearchInput.HaveFocus())
            {
                g_pRenderText->SetFont(g_hFont);
                g_pRenderText->SetTextColor(128, 134, 146, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
                g_pRenderText->RenderText(
                    static_cast<float>(m_Pos.x + TOOLBAR_X + SEARCH_INPUT_X_OFFSET + TOOLBAR_FIELD_TEXT_INSET),
                    static_cast<float>(m_Pos.y + SEARCH_INPUT_Y_OFFSET), L"Search item name...",
                    static_cast<float>(SEARCH_INPUT_WIDTH - 2 * TOOLBAR_FIELD_TEXT_INSET), 0, RT3_SORT_LEFT);
            }
            m_BtnSearch.Render();
            m_BtnReset.Render();
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(190, 196, 208, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<float>(m_Pos.x + TOOLBAR_X + LEVEL_LABEL_X_OFFSET),
                static_cast<float>(m_Pos.y + SEARCH_INPUT_Y_OFFSET), L"Lv.", static_cast<float>(LEVEL_LABEL_WIDTH), 0,
                RT3_SORT_LEFT);
            g_pRenderText->RenderText(static_cast<float>(m_Pos.x + TOOLBAR_X + LEVEL_MINIMUM_X_OFFSET + LEVEL_INPUT_WIDTH),
                static_cast<float>(m_Pos.y + SEARCH_INPUT_Y_OFFSET), L"-", static_cast<float>(LEVEL_RANGE_SEPARATOR_WIDTH),
                0, RT3_SORT_CENTER);
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
        m_SellPanel.Render(m_bHasSellItem ? &m_SellItemSnapshot : nullptr);
    }

    DisableAlphaBlend();

    return true;
}

void SEASON3B::CNewUIAuctionWindow::RenderDetailPanel()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const int x = m_Pos.x + BROWSE_BODY_X;

    if (!m_bHasDetailResponse)
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->RenderText((float)x, (float)(m_Pos.y + DETAIL_CARD_Y + 8), I18N::Game::PleaseWait,
            (float)DETAIL_CARD_WIDTH, 0, RT3_SORT_CENTER);
        return;
    }

    const auto& detail = m_DetailResponse;
    const uint32_t estimatedServerTime = m_ServerClock.EstimatedServerTime(GetTickCount());
    const uint32_t remainingSeconds = detail.EndsAt > estimatedServerTime ? (detail.EndsAt - estimatedServerTime) : 0;
    const auto countdown = AuctionHouse::FormatAuctionCountdown(std::chrono::seconds(remainingSeconds));

    // Item card: the preview in its slot, then who sells it, how long is left and who is bidding.
    EnableAlphaTest();
    RenderContainer(x, m_Pos.y + DETAIL_CARD_Y, DETAIL_CARD_WIDTH, DETAIL_CARD_HEIGHT);
    RenderAuctionItemSlot(m_Pos.x + DETAIL_SLOT_X, m_Pos.y + DETAIL_SLOT_Y, DETAIL_SLOT_SIZE,
        GetAuctionItemSlotTint(m_DetailItem, m_bPointingDetailItem));

    const int textX = m_Pos.x + DETAIL_TEXT_X;
    const int textWidth = x + DETAIL_CARD_WIDTH - textX - 12;
    int lineY = m_Pos.y + DETAIL_CARD_Y + 12;
    if (m_DetailItem != nullptr)
    {
        g_pRenderText->SetFont(g_hFontBold);
        SetBrowseItemNameColor(m_DetailItem);
        const std::wstring itemName = GetItemDisplayName(m_DetailItem);
        g_pRenderText->RenderText((float)textX, (float)lineY, itemName.c_str(), (float)(textWidth - 90), 0, RT3_SORT_LEFT);
    }

    // The listing's status, right-aligned on the name's line.
    int statusR = 190, statusG = 196, statusB = 208;
    switch (detail.Status)
    {
    case AuctionListingStatus::Active: statusR = 120; statusG = 220; statusB = 130; break;
    case AuctionListingStatus::Sold: statusR = 238; statusG = 214; statusB = 120; break;
    case AuctionListingStatus::Cancelled:
    case AuctionListingStatus::AdminRemoved: statusR = 255; statusG = 120; statusB = 110; break;
    default: break;
    }
    RenderCardLine(ListingStatusText(detail.Status), textX, lineY, textWidth, true, statusR, statusG, statusB, RT3_SORT_RIGHT);

    lineY += 20;
    RenderCardLine(std::wstring(I18N::Game::Seller) + L": " + detail.SellerName, textX, lineY, textWidth, false, 170, 176, 190);

    lineY += 16;
    constexpr uint32_t UrgentSeconds = 3600;
    const bool urgent = remainingSeconds < UrgentSeconds;
    RenderCardLine(remainingSeconds == 0 ? std::wstring(L"Ended") : L"Ends in " + countdown, textX, lineY, textWidth, false,
        urgent ? 255 : 196, urgent ? 132 : 200, urgent ? 100 : 210);

    lineY += 16;
    std::wstring bids = detail.BidCount == 0 ? std::wstring(L"No bids yet")
        : std::to_wstring(detail.BidCount) + (detail.BidCount == 1 ? L" bid" : L" bids");
    if (!detail.CurrentBidderName.empty())
    {
        bids += L"  -  highest: " + detail.CurrentBidderName;
    }
    RenderCardLine(bids, textX, lineY, textWidth, false, 196, 200, 210);

    if (!detail.Note.empty())
    {
        lineY += 16;
        RenderCardLine(L"\"" + detail.Note + L"\"", textX, lineY, textWidth, false, 142, 148, 160);
    }

    // Price card: what it costs now on the left, the buyout on the right, each as chips.
    const int half = (DETAIL_CARD_WIDTH - 1) / 2;
    const int cardY = m_Pos.y + DETAIL_PRICE_CARD_Y;
    RenderContainer(x, cardY, DETAIL_CARD_WIDTH, DETAIL_PRICE_CARD_HEIGHT);
    EnableAlphaTest();
    RenderColorQuadARGB(x + half, cardY + 8, 1, DETAIL_PRICE_CARD_HEIGHT - 16, BROWSE_HEADER_LINE_COLOR);
    EndRenderColor();

    RenderCardCaption(detail.BidCount > 0 ? L"CURRENT BID" : L"STARTING PRICE", x, cardY + 9, half, RT3_SORT_CENTER);
    RenderCardCaption(L"BUYOUT", x + half + 1, cardY + 9, half, RT3_SORT_CENTER);

    const AuctionHouse::AuctionOpenResponse* openResponse = m_bHasOpenResponse ? &m_OpenResponse : nullptr;
    const int chipRowY = cardY + DETAIL_CHIP_ROW_Y;
    RenderPriceCellText(x, half, chipRowY,
        LayoutPriceCell(AuctionHouse::PriceComponentsOf(detail.CurrencyMode, detail.CurrentPrice), half, openResponse));
    if (detail.BuyoutPrice.IsZero())
    {
        RenderCardLine(L"No buyout", x + half + 1, chipRowY + BROWSE_ROW_TEXT_Y_OFFSET, half, false, 120, 126, 138,
            RT3_SORT_CENTER);
    }
    else
    {
        RenderPriceCellText(x + half + 1, half, chipRowY,
            LayoutPriceCell(AuctionHouse::PriceComponentsOf(detail.CurrencyMode, detail.BuyoutPrice), half, openResponse));
    }

    if (m_iCurrentTab == TAB_BROWSE && detail.Status == AuctionListingStatus::Active && !detail.CurrentPrice.IsZero())
    {
        const auto minimumBid = AuctionHouse::ComputeMinimumNextBid(detail.CurrencyMode, detail.CurrentPrice, detail.BidCount);
        RenderCardLine(L"Minimum bid: " + PriceSummaryText(detail.CurrencyMode, minimumBid), x + 12, cardY + 76, half - 24,
            false, 142, 148, 160);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderMailboxDetailPanel()
{
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    if (!m_bHasSelectedCollection)
    {
        return;
    }

    const auto& entry = m_SelectedCollection;
    const int x = m_Pos.x + BROWSE_BODY_X;

    // What it is and where it came from: a collected item shows its preview, money shows no slot.
    EnableAlphaTest();
    RenderContainer(x, m_Pos.y + DETAIL_CARD_Y, DETAIL_CARD_WIDTH, DETAIL_CARD_HEIGHT);
    int textX = x + 14;
    if (entry.HasItem)
    {
        RenderAuctionItemSlot(m_Pos.x + DETAIL_SLOT_X, m_Pos.y + DETAIL_SLOT_Y, DETAIL_SLOT_SIZE,
            GetAuctionItemSlotTint(m_DetailItem, m_bPointingDetailItem));
        textX = m_Pos.x + DETAIL_TEXT_X;
    }
    const int textWidth = x + DETAIL_CARD_WIDTH - textX - 12;
    int lineY = m_Pos.y + DETAIL_CARD_Y + 14;
    RenderCardLine(CollectionKindText(entry.CollectionKind), textX, lineY, textWidth - 110, true, 238, 238, 244);
    const bool claimed = entry.CollectionStatus == AuctionCollectionStatus::Claimed;
    RenderCardLine(CollectionStatusText(entry.CollectionStatus), textX, lineY, textWidth, true, claimed ? 150 : 238,
        claimed ? 156 : 214, claimed ? 168 : 120, RT3_SORT_RIGHT);

    lineY += 20;
    wchar_t source[96];
    mu_swprintf(source, L"%ls: #%llu", I18N::Game::AuctionSource, entry.SourceListingId);
    RenderCardLine(source, textX, lineY, textWidth, false, 170, 176, 190);
    if (entry.HasItem && m_DetailItem != nullptr)
    {
        lineY += 16;
        SetBrowseItemNameColor(m_DetailItem);
        g_pRenderText->SetFont(g_hFontBold);
        const std::wstring itemName = GetItemDisplayName(m_DetailItem);
        g_pRenderText->RenderText((float)textX, (float)lineY, itemName.c_str(), (float)textWidth, 0, RT3_SORT_LEFT);
    }

    // Money: the amount waiting, as chips, and what is left to claim by field below.
    const bool isMoney = !entry.HasItem;
    if (isMoney)
    {
        const int cardY = m_Pos.y + AMOUNT_CARD_Y;
        RenderContainer(x, cardY, DETAIL_CARD_WIDTH, AMOUNT_CARD_HEIGHT);
        RenderCardCaption(L"AVAILABLE TO COLLECT", x + 12, cardY + 9, 200, RT3_SORT_LEFT);
        if (entry.CollectionStatus == AuctionCollectionStatus::PartiallyClaimed)
        {
            RenderCardLine(L"Originally " + PriceSummaryText(entry.CurrencyMode, entry.Original), x + 12, cardY + 9,
                DETAIL_CARD_WIDTH - 24, false, 142, 148, 160, RT3_SORT_RIGHT);
        }
        RenderPriceCellText(x, DETAIL_CARD_WIDTH, m_Pos.y + AMOUNT_CARD_Y + AMOUNT_CHIP_ROW_Y,
            LayoutPriceCell(AuctionHouse::PriceComponentsOf(entry.CurrencyMode, entry.Remaining), DETAIL_CARD_WIDTH,
                m_bHasOpenResponse ? &m_OpenResponse : nullptr));
    }

    const wchar_t* fruitLabels[] = {
        I18N::Game::Strength, I18N::Game::Agility, I18N::Game::Vitality, I18N::Game::Energy, I18N::Game::Command
    };
    if (isMoney && entry.CurrencyMode != AuctionCurrencyMode::Zen)
    {
        RenderContainer(x, m_Pos.y + CLAIM_CARD_Y, DETAIL_CARD_WIDTH, CLAIM_CARD_HEIGHT);
        const size_t inputCount = entry.CurrencyMode == AuctionCurrencyMode::Fruits ? 5 : 1;
        for (size_t i = 0; i < inputCount; ++i)
        {
            const int fieldX = m_Pos.x + CLAIM_FIELD_X + static_cast<int>(i) * CLAIM_INPUT_COLUMN_WIDTH;
            const wchar_t* label = inputCount == 1 ? I18N::Game::AuctionClaimAmount : fruitLabels[i];
            RenderCardCaption(label, fieldX, m_Pos.y + CLAIM_LABEL_Y, CLAIM_INPUT_COLUMN_WIDTH, RT3_SORT_LEFT);
            const UI::NineSlice::StateTints& tints = AuctionFieldSkin().tints;
            UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(fieldX),
                static_cast<float>(m_Pos.y + CLAIM_FIELD_Y), static_cast<float>(CLAIM_FIELD_WIDTH), 23.f,
                m_CollectionClaimInputs[i].HaveFocus() ? tints.pressed : tints.normal);
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
        RenderCardLine(I18N::Game::AuctionInvalidClaim, x + 4, m_Pos.y + CLAIM_CARD_Y + CLAIM_CARD_HEIGHT + 6,
            DETAIL_CARD_WIDTH, false, 255, 110, 100);
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
            const wchar_t* message = m_LastOperationMessage.empty()
                ? (succeeded ? I18N::Game::Success : I18N::Game::Failed)
                : m_LastOperationMessage.c_str();
            g_pRenderText->RenderText((float)m_Pos.x, (float)(m_Pos.y + OPERATION_RESULT_Y_OFFSET),
                message, (float)WINDOW_WIDTH, 0, RT3_SORT_CENTER);
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
    // Previous/Next always render fully interactive - plain hover/press/release, same as Search/Reset - and
    // are never grayed out or Lock()'d at a page boundary. A click at a boundary already does nothing
    // functionally (guarded in the click handlers below by m_CurrentPage comparisons); it just doesn't also
    // need to look disabled, which previously made Next look stuck gray the instant a click reached the last
    // page (locking suppresses all further hover/press updates, so the gray tint never clears again).
    const bool hasResponse = m_iCurrentTab == TAB_MAILBOX ? m_bHasMailboxResponse : m_bHasListingResponse;
    const uint16_t knownTotalPages = m_iCurrentTab == TAB_MAILBOX
        ? m_MailboxResponse.TotalPages : m_ListingResponse.TotalPages;

    m_BtnPrevPage.Render();
    m_BtnNextPage.Render();

    const uint16_t totalPages = hasResponse ? std::max<uint16_t>(knownTotalPages, 1) : 1;
    wchar_t pageText[32];
    mu_swprintf(pageText, L"%d / %d", m_CurrentPage, totalPages);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText((float)(m_Pos.x + PageClusterX(m_iCurrentTab) + PAGE_BTN_WIDTH), (float)(m_Pos.y + PAGE_BTN_Y_OFFSET + PAGE_LABEL_Y_OFFSET),
        pageText, (float)PAGE_CLUSTER_GAP, 0, RT3_SORT_CENTER);
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
            int iconX = 0;
            int iconY = 0;
            int iconSize = 0;
            m_SellPanel.GetItemIconRect(iconX, iconY, iconSize);
            RenderItem3DFittedToSlot((float)iconX, (float)iconY, (float)iconSize, (float)iconSize,
                m_SellItemSnapshot.Type, m_SellItemSnapshot.Level, m_SellItemSnapshot.ExcellentFlags,
                m_SellItemSnapshot.AncientDiscriminator, BROWSE_ICON_FILL, false);
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
            const int iconSize = DETAIL_SLOT_SIZE - 2 * DETAIL_ICON_INSET;
            RenderItem3DFittedToSlot((float)(m_Pos.x + DETAIL_SLOT_X + DETAIL_ICON_INSET),
                (float)(m_Pos.y + DETAIL_SLOT_Y + DETAIL_ICON_INSET), (float)iconSize, (float)iconSize,
                m_DetailItem->Type, m_DetailItem->Level, m_DetailItem->ExcellentFlags, m_DetailItem->AncientDiscriminator,
                BROWSE_ICON_FILL, false);
        }

        // The price chips' icons, laid out by the same function that places their amounts.
        if (m_iCurrentTab == TAB_MAILBOX && m_bHasSelectedCollection && !m_SelectedCollection.HasItem)
        {
            RenderPriceChipIcons(m_Pos.x + BROWSE_BODY_X, m_Pos.y + AMOUNT_CARD_Y + AMOUNT_CHIP_ROW_Y,
                LayoutPriceCell(AuctionHouse::PriceComponentsOf(m_SelectedCollection.CurrencyMode, m_SelectedCollection.Remaining),
                    DETAIL_CARD_WIDTH, m_bHasOpenResponse ? &m_OpenResponse : nullptr));
        }
        else if (m_iCurrentTab != TAB_MAILBOX && m_bHasDetailResponse)
        {
            const AuctionHouse::AuctionOpenResponse* openResponse = m_bHasOpenResponse ? &m_OpenResponse : nullptr;
            const int half = (DETAIL_CARD_WIDTH - 1) / 2;
            const int chipRowY = m_Pos.y + DETAIL_PRICE_CARD_Y + DETAIL_CHIP_ROW_Y;
            RenderPriceChipIcons(m_Pos.x + BROWSE_BODY_X, chipRowY,
                LayoutPriceCell(AuctionHouse::PriceComponentsOf(m_DetailResponse.CurrencyMode, m_DetailResponse.CurrentPrice),
                    half, openResponse));
            RenderPriceChipIcons(m_Pos.x + BROWSE_BODY_X + half + 1, chipRowY,
                LayoutPriceCell(AuctionHouse::PriceComponentsOf(m_DetailResponse.CurrencyMode, m_DetailResponse.BuyoutPrice),
                    half, openResponse));
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
    const TableLayout table = GetTableLayout(m_iCurrentTab == TAB_BROWSE);
    for (size_t row = 0; row < rowCount; ++row)
    {
        const ITEM* item = m_RowItems[row];
        if (item == nullptr)
        {
            continue;
        }

        const int iconX = m_Pos.x + RowSlotX(m_iCurrentTab) + BROWSE_ICON_CELL_OFFSET + BROWSE_ICON_MARGIN;
        const int iconY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT + BROWSE_ICON_CELL_OFFSET
            + BROWSE_ICON_MARGIN;
        if (!IsCoveredByOpenDropdown(iconX, iconY, BROWSE_ICON_SIZE, BROWSE_ICON_SIZE))
        {
            RenderItem3DFittedToSlot((float)iconX, (float)iconY, (float)BROWSE_ICON_SIZE, (float)BROWSE_ICON_SIZE,
                item->Type, item->Level, item->ExcellentFlags, item->AncientDiscriminator, BROWSE_ICON_FILL, false);
        }

        // The icons of each price chip, laid out by the same function that places the amounts (see
        // RenderBrowseTab). Only the Browse/My Listings table has price columns; Mailbox's layout does not.
        if ((m_iCurrentTab == TAB_BROWSE || m_iCurrentTab == TAB_MY_LISTINGS) && m_bHasListingResponse
            && m_BrowseScrollOffset + row < m_ListingResponse.Listings.size())
        {
            const auto& rowListing = m_ListingResponse.Listings[m_BrowseScrollOffset + row];
            const int rowY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT;
            const auto renderPriceIcons = [&](const AuctionHouse::AuctionAmount& amount, int cellX, int cellWidth)
            {
                const auto chips = LayoutPriceCell(AuctionHouse::PriceComponentsOf(rowListing.CurrencyMode, amount),
                    cellWidth, m_bHasOpenResponse ? &m_OpenResponse : nullptr);
                for (const PriceChip& chip : chips)
                {
                    if (chip.icon
                        && !IsCoveredByOpenDropdown(cellX + chip.x, rowY + chip.textY + PRICE_ICON_Y_OFFSET, PRICE_ICON_SIZE, PRICE_ICON_SIZE))
                    {
                        RenderItem3DFittedToSlot(static_cast<float>(cellX + chip.x),
                            static_cast<float>(rowY + chip.textY + PRICE_ICON_Y_OFFSET),
                            static_cast<float>(PRICE_ICON_SIZE), static_cast<float>(PRICE_ICON_SIZE),
                            chip.icon->itemType, chip.icon->itemLevel, 0, 0, PRICE_ICON_FILL, false);
                    }
                }
            };
            renderPriceIcons(rowListing.CurrentPrice, m_Pos.x + table.innerX() + table.priceX(), TABLE_PRICE_COLUMN_WIDTH);
            renderPriceIcons(rowListing.BuyoutPrice, m_Pos.x + table.innerX() + table.buyoutX(), TABLE_BUYOUT_COLUMN_WIDTH);
        }
    }

    if (m_iCurrentTab == TAB_BROWSE && m_bHasOpenResponse)
    {
        const WalletView view = BuildWalletView(m_OpenResponse);
        const WalletLayout layout = ComputeWalletLayout(view);
        RenderWalletIcons(m_Pos.x, m_Pos.y, layout.jewelsY, view.jewels, WALLET_JEWEL_COLUMNS, WALLET_JEWEL_ICON_SIZE);
        RenderWalletIcons(m_Pos.x, m_Pos.y, layout.fruitsY, view.fruits, WALLET_FRUIT_COLUMNS, WALLET_FRUIT_ICON_SIZE);
    }

    // Deferred to the shared 3D render manager so the tooltip draws after every window's own icons, the same
    // ordering CNewUIMyInventory::Render3D relies on for its own item tooltip.
    if (m_iPointedItemRow != -1 && m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->RenderUI2DEffect(INFORMATION_CAMERA_Z_ORDER, UI2DEffectCallback, this, static_cast<DWORD>(m_iPointedItemRow), 0);
    }
}

bool SEASON3B::CNewUIAuctionWindow::IsCoveredByOpenDropdown(int x, int y, int width, int height) const
{
    const CNewUIComboBox* const combos[] = { &m_CurrencyCombo, &m_CategoryCombo, &m_StatusCombo, &m_CollectionKindCombo };
    for (const CNewUIComboBox* combo : combos)
    {
        int listX = 0;
        int listY = 0;
        int listWidth = 0;
        int listHeight = 0;
        if (combo->GetOpenListRect(listX, listY, listWidth, listHeight)
            && x < listX + listWidth && x + width > listX && y < listY + listHeight && y + height > listY)
        {
            return true;
        }
    }
    return false;
}

void SEASON3B::CNewUIAuctionWindow::RenderSellItemTooltip() const
{
    if (!m_bHasSellItem)
    {
        return;
    }

    int iconX = 0;
    int iconY = 0;
    int iconSize = 0;
    m_SellPanel.GetItemIconRect(iconX, iconY, iconSize);
    RenderItemInfo(iconX + iconSize / 2, iconY + iconSize / 2, const_cast<ITEM*>(&m_SellItemSnapshot), false, TOOLTIP_TYPE_AUCTION);
}

void SEASON3B::CNewUIAuctionWindow::RenderBrowseHeader()
{
    const TableLayout table = GetTableLayout(m_iCurrentTab == TAB_BROWSE, m_iCurrentTab == TAB_MY_LISTINGS);
    EnableAlphaTest();
    RenderTableContainer(m_Pos.x + table.x, m_Pos.y + BROWSE_HEADER_Y, table.width);
    RenderColorQuadARGB(m_Pos.x + table.innerX(), m_Pos.y + BROWSE_BODY_Y - 1, table.innerWidth(), 1,
        BROWSE_HEADER_LINE_COLOR);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const int headerTextY = m_Pos.y + BROWSE_HEADER_Y + 5;
    const int innerX = m_Pos.x + table.innerX();
    g_pRenderText->RenderText((float)(innerX + BROWSE_TEXT_X_OFFSET), (float)headerTextY, I18N::Game::ItemName,
        (float)(table.itemWidth() - BROWSE_TEXT_X_OFFSET - 4), 0, RT3_SORT_LEFT);

    const int timeDirection = m_iCurrentTab == TAB_BROWSE && m_SelectedSort == AuctionSort::EndingSoonest ? 1 : 0;
    RenderSortableHeader(innerX + table.timeX(), TABLE_TIME_COLUMN_WIDTH, headerTextY, I18N::Game::TimeLeft,
        timeDirection);
    if (table.statusColumn)
    {
        g_pRenderText->RenderText((float)(innerX + table.statusX()), (float)headerTextY, I18N::Game::Status,
            (float)TABLE_STATUS_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
    }

    int priceDirection = 0;
    if (m_iCurrentTab == TAB_BROWSE && m_SelectedSort == AuctionSort::PriceAscending)
    {
        priceDirection = 1;
    }
    else if (m_iCurrentTab == TAB_BROWSE && m_SelectedSort == AuctionSort::PriceDescending)
    {
        priceDirection = -1;
    }
    RenderSortableHeader(innerX + table.priceX(), TABLE_PRICE_COLUMN_WIDTH, headerTextY, L"Current Price", priceDirection);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText((float)(innerX + table.buyoutX()), (float)headerTextY, I18N::Game::Buyout,
        (float)TABLE_BUYOUT_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIAuctionWindow::RenderBrowseTab()
{
    const TableLayout table = GetTableLayout(m_iCurrentTab == TAB_BROWSE, m_iCurrentTab == TAB_MY_LISTINGS);
    // Nine compact rows. Item icons remain in the engine's 3D pass; the cards, text and price amounts are ordinary
    // UI rendering. The countdown uses m_ServerClock's live estimate, not the frozen response time.
    const int innerX = m_Pos.x + table.innerX();

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    if (!m_bHasListingResponse)
    {
        g_pRenderText->RenderText((float)innerX, (float)(m_Pos.y + BROWSE_BODY_Y + 8), I18N::Game::PleaseWait,
            (float)table.innerWidth(), 0, RT3_SORT_CENTER);
        return;
    }

    if (m_ListingResponse.Listings.empty())
    {
        g_pRenderText->RenderText((float)innerX, (float)(m_Pos.y + BROWSE_BODY_Y + 8), I18N::Game::NoListingsFound,
            (float)table.innerWidth(), 0, RT3_SORT_CENTER);
        return;
    }

    const uint32_t estimatedServerTime = m_ServerClock.EstimatedServerTime(GetTickCount());
    const auto remainingRows = m_ListingResponse.Listings.size() - std::min(m_BrowseScrollOffset, m_ListingResponse.Listings.size());
    const auto rowCount = std::min(MaxBrowseRows, remainingRows);
    const AuctionHouse::AuctionOpenResponse* openResponse = m_bHasOpenResponse ? &m_OpenResponse : nullptr;
    for (size_t row = 0; row < rowCount; ++row)
    {
        const auto& listing = m_ListingResponse.Listings[m_BrowseScrollOffset + row];
        const int rowY = m_Pos.y + BROWSE_BODY_Y + static_cast<int>(row) * BROWSE_ROW_HEIGHT;
        const bool isHovered = static_cast<int>(row) == m_iPointedRow;

        // The card runs from the middle of the slot to the table's right edge; the slot, drawn over its left end,
        // keeps each preview contained and shows its rarity before the user opens the tooltip. Every slot shares
        // the same dark interior; only its thin ring is tinted.
        EnableAlphaTest();
        RenderRowCard(innerX + ROW_CARD_START, rowY, table.innerWidth() - ROW_CARD_START, isHovered);
        RenderAuctionItemSlot(innerX + BROWSE_ICON_CELL_OFFSET, rowY + BROWSE_ICON_CELL_OFFSET, BROWSE_ICON_CELL_SIZE,
            GetAuctionItemSlotTint(m_RowItems[row], isHovered));

        const auto remainingSeconds = listing.EndsAt > estimatedServerTime ? (listing.EndsAt - estimatedServerTime) : 0;
        const auto countdown = AuctionHouse::FormatAuctionCountdown(std::chrono::seconds(remainingSeconds));
        const std::wstring itemName = m_RowItems[row] != nullptr ? GetItemDisplayName(m_RowItems[row]) : std::wstring();

        // Item cell: the name, with the seller under it on Browse (My Listings shows its own status column).
        const bool showSeller = m_iCurrentTab == TAB_BROWSE;
        const int textX = innerX + BROWSE_TEXT_X_OFFSET;
        const int textWidth = table.itemWidth() - BROWSE_TEXT_X_OFFSET - 4;
        g_pRenderText->SetFont(g_hFontBold);
        SetBrowseItemNameColor(m_RowItems[row]);
        g_pRenderText->RenderText((float)textX, (float)(rowY + (showSeller ? ROW_NAME_Y_OFFSET : BROWSE_ROW_TEXT_Y_OFFSET)),
            itemName.c_str(), (float)textWidth, 0, RT3_SORT_LEFT);
        g_pRenderText->SetFont(g_hFont);
        if (showSeller)
        {
            g_pRenderText->SetTextColor(142, 148, 160, 255);
            g_pRenderText->RenderText((float)textX, (float)(rowY + ROW_SUB_Y_OFFSET), listing.SellerName.c_str(),
                (float)textWidth, 0, RT3_SORT_LEFT);
        }

        // Time left turns warm in its last hour so an ending auction stands out in the list.
        constexpr uint32_t UrgentSeconds = 3600;
        if (remainingSeconds < UrgentSeconds)
        {
            g_pRenderText->SetTextColor(255, 132, 100, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(196, 200, 210, 255);
        }
        g_pRenderText->RenderText((float)(innerX + table.timeX()), (float)(rowY + BROWSE_ROW_TEXT_Y_OFFSET),
            countdown.c_str(), (float)TABLE_TIME_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        if (table.statusColumn)
        {
            g_pRenderText->SetTextColor(196, 200, 210, 255);
            g_pRenderText->RenderText((float)(innerX + table.statusX()), (float)(rowY + BROWSE_ROW_TEXT_Y_OFFSET),
                ListingStatusText(listing.Status), (float)TABLE_STATUS_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        }

        // Asking prices: one chip per component (amounts here, icons in Render3D). A price paid in several
        // things at once is shown in full - never as a label - because it is what the buyer needs to see.
        RenderPriceCellText(innerX + table.priceX(), TABLE_PRICE_COLUMN_WIDTH, rowY,
            LayoutPriceCell(AuctionHouse::PriceComponentsOf(listing.CurrencyMode, listing.CurrentPrice),
                TABLE_PRICE_COLUMN_WIDTH, openResponse));
        RenderPriceCellText(innerX + table.buyoutX(), TABLE_BUYOUT_COLUMN_WIDTH, rowY,
            LayoutPriceCell(AuctionHouse::PriceComponentsOf(listing.CurrencyMode, listing.BuyoutPrice),
                TABLE_BUYOUT_COLUMN_WIDTH, openResponse));
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderCategoryRail()
{
    // A persistent category rail makes filtering a single, visible action instead of hiding the server's
    // seven real auction categories inside a dropdown. It deliberately uses the existing MU table materials
    // rather than borrowing the reference game's artwork.
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const SIZE lineSize = g_pRenderText->MeasureText(L"Ag", 2);
    constexpr int RowHeight = CATEGORY_RAIL_ROW_HEIGHT - 2 * CATEGORY_RAIL_ROW_INSET;
    const int textOffsetY = CATEGORY_RAIL_ROW_INSET + (RowHeight - lineSize.cy) / 2;
    for (int category = 0; category < static_cast<int>(std::size(m_CategoryLabels)); ++category)
    {
        const int y = m_Pos.y + CATEGORY_RAIL_Y + category * CATEGORY_RAIL_ROW_HEIGHT;
        const bool selected = category == m_SelectedCategoryIndex;
        const bool hovered = !selected && CheckMouseIn(m_Pos.x + CATEGORY_RAIL_X, y, CATEGORY_RAIL_WIDTH, CATEGORY_RAIL_ROW_HEIGHT);
        UI::NineSlice::Render(AuctionButtonSkin().texture, static_cast<float>(m_Pos.x + CATEGORY_RAIL_X),
            static_cast<float>(y + CATEGORY_RAIL_ROW_INSET), static_cast<float>(CATEGORY_RAIL_WIDTH),
            static_cast<float>(RowHeight),
            selected ? CATEGORY_TINT_SELECTED : (hovered ? CATEGORY_TINT_HOVER : CATEGORY_TINT_IDLE));

        g_pRenderText->SetTextColor(selected ? 255 : 205, selected ? 215 : 205, selected ? 100 : 205, 255);
        g_pRenderText->RenderText((float)(m_Pos.x + CATEGORY_RAIL_X + 8), (float)(y + textOffsetY),
            m_CategoryLabels[category], (float)(CATEGORY_RAIL_WIDTH - 12), 0, RT3_SORT_LEFT);
        g_pRenderText->RenderText((float)(m_Pos.x + CATEGORY_RAIL_X + CATEGORY_RAIL_WIDTH - 16), (float)(y + textOffsetY),
            L">", 12.0f, 0, RT3_SORT_LEFT);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderCurrencyBalances()
{
    if (!m_bHasOpenResponse)
    {
        return;
    }

    const WalletView view = BuildWalletView(m_OpenResponse);
    if (view.jewels.empty() && view.fruits.empty())
    {
        return;
    }

    const WalletLayout layout = ComputeWalletLayout(view);
    const auto renderPanel = [this](int y, int height)
    {
        UI::NineSlice::Render(AuctionPanelSkin().texture, static_cast<float>(m_Pos.x + WALLET_X),
            static_cast<float>(m_Pos.y + y), static_cast<float>(WALLET_WIDTH), static_cast<float>(height),
            AuctionPanelSkin().tints.normal);
    };
    if (!view.jewels.empty())
    {
        renderPanel(layout.jewelsY, layout.jewelsHeight);
    }
    if (!view.fruits.empty())
    {
        renderPanel(layout.fruitsY, layout.fruitsHeight);
    }
    if (view.hasZen)
    {
        renderPanel(layout.zenY, WALLET_ZEN_HEIGHT);
    }

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(235, 210, 120, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    RenderWalletCounts(m_Pos.x, m_Pos.y, layout.jewelsY, view.jewels, WALLET_JEWEL_COLUMNS);
    RenderWalletCounts(m_Pos.x, m_Pos.y, layout.fruitsY, view.fruits, WALLET_FRUIT_COLUMNS);

    if (view.hasZen)
    {
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x + WALLET_X),
            static_cast<float>(m_Pos.y + layout.zenY + WALLET_ZEN_TITLE_Y_OFFSET), L"ZEN",
            static_cast<float>(WALLET_WIDTH), 0, RT3_SORT_CENTER);
        const std::wstring amount = FormatWalletAmount(view.zen);
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x + WALLET_X + 4),
            static_cast<float>(m_Pos.y + layout.zenY + WALLET_ZEN_VALUE_Y_OFFSET), amount.c_str(),
            static_cast<float>(WALLET_WIDTH - 8), 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CNewUIAuctionWindow::RenderMailboxHeader()
{
    EnableAlphaTest();
    RenderTableContainer(m_Pos.x + BROWSE_BODY_X, m_Pos.y + BROWSE_HEADER_Y, BROWSE_CONTENT_WIDTH);
    RenderColorQuadARGB(m_Pos.x + BROWSE_BODY_X + TABLE_INSET, m_Pos.y + BROWSE_BODY_Y - 1,
        BROWSE_CONTENT_WIDTH - 2 * TABLE_INSET, 1, BROWSE_HEADER_LINE_COLOR);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const float y = static_cast<float>(m_Pos.y + BROWSE_HEADER_Y + 5);
    g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + TABLE_INSET + 5), y, I18N::Game::Mailbox,
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
        const bool isHovered = m_iPointedRow == static_cast<int>(row);
        EnableAlphaTest();
        RenderRowCard(m_Pos.x + BROWSE_BODY_X + TABLE_INSET, y, BROWSE_CONTENT_WIDTH - 2 * TABLE_INSET, isHovered);
        if (entry.HasItem)
        {
            EnableAlphaTest();
            RenderAuctionItemSlot(m_Pos.x + BROWSE_BODY_X + MAILBOX_CONTENT_COLUMN_X + BROWSE_ICON_CELL_OFFSET,
                y + BROWSE_ICON_CELL_OFFSET, BROWSE_ICON_CELL_SIZE, GetAuctionItemSlotTint(m_RowItems[row], isHovered));
        }

        g_pRenderText->SetTextColor(220, 220, 220, 255);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + TABLE_INSET + 5), (float)(y + BROWSE_ROW_TEXT_Y_OFFSET),
            CollectionKindText(entry.CollectionKind), (float)(MAILBOX_KIND_COLUMN_WIDTH - 10), 0, RT3_SORT_LEFT);
        const auto contents = entry.HasItem ? std::wstring(I18N::Game::Item)
            : AuctionAmountText(entry.CurrencyMode, entry.Remaining);
        const int contentTextX = m_Pos.x + BROWSE_BODY_X + MAILBOX_CONTENT_COLUMN_X
            + (entry.HasItem ? BROWSE_TEXT_X_OFFSET : 5);
        g_pRenderText->RenderText((float)contentTextX, (float)(y + BROWSE_ROW_TEXT_Y_OFFSET), contents.c_str(),
            (float)(MAILBOX_CONTENT_COLUMN_WIDTH - (entry.HasItem ? BROWSE_TEXT_X_OFFSET : 10)), 0, RT3_SORT_LEFT);
        wchar_t source[32];
        mu_swprintf(source, L"%llu", entry.SourceListingId);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_SOURCE_COLUMN_X), (float)(y + BROWSE_ROW_TEXT_Y_OFFSET),
            source, (float)MAILBOX_SOURCE_COLUMN_WIDTH, 0, RT3_SORT_CENTER);
        g_pRenderText->RenderText((float)(m_Pos.x + BROWSE_BODY_X + MAILBOX_STATUS_COLUMN_X), (float)(y + BROWSE_ROW_TEXT_Y_OFFSET),
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
    return WINDOW_WIDTH;
}

void SEASON3B::CNewUIAuctionWindow::ComputeTabLayout(int (&outX)[TAB_COUNT], int (&outWidth)[TAB_COUNT]) const
{
    int x = m_Pos.x + TAB_REGION_X;
    for (int tab = 0; tab < TAB_COUNT; ++tab)
    {
        outWidth[tab] = TAB_WIDTH;
        outX[tab] = x;
        x += outWidth[tab] + TAB_GAP;
    }
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
    LoadBitmap(L"Interface\\newui_auction_item_slot.tga", IMAGE_AUCTION_ITEM_SLOT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_auction_button.tga", IMAGE_AUCTION_BUTTON, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_auction_field.tga", IMAGE_AUCTION_FIELD, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_auction_row.tga", IMAGE_AUCTION_ROW, GL_LINEAR);
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
    DeleteBitmap(IMAGE_AUCTION_ITEM_SLOT);
    DeleteBitmap(IMAGE_AUCTION_BUTTON);
    DeleteBitmap(IMAGE_AUCTION_FIELD);
    DeleteBitmap(IMAGE_AUCTION_ROW);
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
