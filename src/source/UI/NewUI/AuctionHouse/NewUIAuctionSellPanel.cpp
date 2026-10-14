// NewUIAuctionSellPanel.cpp: the form of the Auction House's Sell tab.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionSellPanel.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "UI/NewUI/Widgets/NineSlice.h"
#include "UI/NewUI/NewUISystem.h"
#include "Engine/Object/ZzzInventory.h"
#include "I18N/All.h"
#include "Audio/DSPlaySound.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Core/Text/TextLineWrap.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace SEASON3B;
using namespace UI::AuctionChrome;

namespace
{
    // Everything is positioned in the Sell window's own coordinates. The window is the same 600 x 429 as Browse and
    // laid out the same way: a narrow rail on the left (here the item and the fees, there the categories and the
    // wallet) and the form to its right where Browse has its table. Rows start at the toolbar line (y 73).
    constexpr int WINDOW_WIDTH = SEASON3B::CNewUIAuctionWindow::WINDOW_WIDTH;
    constexpr int CONTENT_X = 11;
    constexpr int RAIL_X = CONTENT_X;
    constexpr int RAIL_WIDTH = 114;
    constexpr int FORM_X = 132;
    constexpr int FORM_WIDTH = WINDOW_WIDTH - CONTENT_X - FORM_X;
    constexpr int FORM_INNER_RIGHT = FORM_X + FORM_WIDTH - 8;
    constexpr int FIELD_HEIGHT = 23;
    // Text tops that centre the UI font's 8-pixel text box in a 23-pixel field, and the gap between a field's frame
    // and the typed text inside it (the same values the Browse toolbar uses).
    constexpr int TEXT_Y_OFFSET = 7;
    constexpr int INPUT_INSET = 6;
    constexpr int INPUT_HEIGHT = 16;
    constexpr int TEXT_LINE_HEIGHT = 11;

    constexpr int TOP_Y = 73;
    constexpr int ITEM_CARD_Y = TOP_Y;
    constexpr int ITEM_CARD_HEIGHT = 150;
    constexpr int ITEM_SLOT_SIZE = 56;
    constexpr int ITEM_SLOT_X = RAIL_X + (RAIL_WIDTH - ITEM_SLOT_SIZE) / 2;
    constexpr int ITEM_SLOT_Y = ITEM_CARD_Y + 10;
    constexpr int ITEM_ICON_INSET = 4;
    constexpr int ITEM_NAME_Y = ITEM_CARD_Y + 74;
    constexpr int ITEM_NAME_MAX_LINES = 2;
    constexpr int ITEM_HINT_Y = ITEM_CARD_Y + 102;
    constexpr int ITEM_HINT_MAX_LINES = 4;
    constexpr int RAIL_TEXT_PAD = 6;

    constexpr int FEE_CARD_Y = ITEM_CARD_Y + ITEM_CARD_HEIGHT + 6;
    constexpr int FEE_CARD_HEIGHT = 102;
    constexpr int FEE_TITLE_Y = FEE_CARD_Y + 8;
    constexpr int FEE_LISTING_LABEL_Y = FEE_CARD_Y + 25;
    constexpr int FEE_SUCCESS_LABEL_Y = FEE_CARD_Y + 61;

    constexpr int MODE_Y = TOP_Y;
    constexpr int MODE_HEIGHT = FIELD_HEIGHT;
    constexpr int MODE_GAP = 4;
    constexpr int MODE_WIDTH = (FORM_WIDTH - MODE_GAP) / 2;

    constexpr int PRICE_PANEL_Y = 102;
    constexpr int PRICE_PANEL_HEIGHT = 190;
    constexpr int PRICE_DESCRIPTION_Y = PRICE_PANEL_Y + 7;
    constexpr int ROWS_Y = PRICE_PANEL_Y + 38;
    constexpr int ROW_HEADER_Y = ROWS_Y - 14;
    constexpr int ROW_PITCH = 30;
    constexpr int PRICE_LABEL_X = FORM_X + 8;

    // An auction row: currency, starting amount, optional buyout amount, remove box.
    constexpr int AUCTION_KIND_X = FORM_X + 8;
    constexpr int AUCTION_KIND_WIDTH = 150;
    constexpr int AUCTION_QUANTITY_X = AUCTION_KIND_X + AUCTION_KIND_WIDTH + 8;
    constexpr int AUCTION_QUANTITY_WIDTH = 110;
    constexpr int AUCTION_BUYOUT_X = AUCTION_QUANTITY_X + AUCTION_QUANTITY_WIDTH + 8;
    constexpr int AUCTION_BUYOUT_WIDTH = 110;
    constexpr int AUCTION_REMOVE_X = AUCTION_BUYOUT_X + AUCTION_BUYOUT_WIDTH + 8;

    constexpr int BUYNOW_KIND_X = FORM_X + 8;
    constexpr int BUYNOW_KIND_WIDTH = 190;
    constexpr int BUYNOW_QUANTITY_X = BUYNOW_KIND_X + BUYNOW_KIND_WIDTH + 8;
    constexpr int BUYNOW_QUANTITY_WIDTH = 190;
    constexpr int BUYNOW_REMOVE_X = BUYNOW_QUANTITY_X + BUYNOW_QUANTITY_WIDTH + 8;
    constexpr int BUYNOW_REMOVE_SIZE = FIELD_HEIGHT;

    constexpr int OPTIONS_CARD_Y = PRICE_PANEL_Y + PRICE_PANEL_HEIGHT + 8;
    constexpr int OPTIONS_CARD_HEIGHT = 44;
    constexpr int OPTIONS_Y = OPTIONS_CARD_Y + (OPTIONS_CARD_HEIGHT - FIELD_HEIGHT) / 2;
    constexpr int DURATION_LABEL_X = FORM_X + 8;
    constexpr int DURATION_X = DURATION_LABEL_X + 56;
    constexpr int DURATION_WIDTH = 100;
    constexpr int NOTE_LABEL_X = DURATION_X + DURATION_WIDTH + 16;
    constexpr int NOTE_X = NOTE_LABEL_X + 34;
    constexpr int NOTE_WIDTH = FORM_INNER_RIGHT - NOTE_X;
    constexpr int NOTE_MAX_CHARACTERS = 40;

    constexpr int STATUS_Y = OPTIONS_CARD_Y + OPTIONS_CARD_HEIGHT + 10;

    constexpr int BUTTON_HEIGHT = 23;
    constexpr int BUTTON_Y = 429 - 45 + (45 - BUTTON_HEIGHT) / 2;
    constexpr int BUTTON_MARGIN = 20;
    constexpr int CREATE_WIDTH = 130;
    constexpr int CLEAR_WIDTH = 80;

    constexpr int DEFAULT_DURATION_HOURS = 24;

    const DWORD CARD_TINT = RGBA(104, 110, 122, 255);
    const DWORD MODE_TINT_SELECTED = RGBA(255, 214, 118, 255);
    const DWORD MODE_TINT_HOVER = RGBA(206, 212, 224, 255);
    const DWORD MODE_TINT_IDLE = RGBA(150, 156, 168, 255);
    const DWORD REMOVE_TINT = RGBA(168, 100, 96, 255);

    const wchar_t* const DurationLabelsAll[4] = { L"6 hours", L"12 hours", L"24 hours", L"48 hours" };
    constexpr int DurationHoursAll[4] = { 6, 12, 24, 48 };

    // Renders left-aligned text of the given colour with its top at y.
    void RenderLabel(const wchar_t* text, int x, int y, int width, DWORD colour, bool bold)
    {
        g_pRenderText->SetFont(bold ? g_hFontBold : g_hFont);
        g_pRenderText->SetTextColor((colour >> 16) & 0xFF, (colour >> 8) & 0xFF, colour & 0xFF, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(y), text, static_cast<float>(width), 0,
            RT3_SORT_LEFT);
    }

    // Draws text wrapped to the width, one line per row of lineHeight, centred or left-aligned, up to maxLines
    // (anything beyond is dropped). Returns the number of lines drawn.
    int RenderWrapped(const std::wstring& text, int x, int y, int width, DWORD colour, bool bold, bool centred,
        int maxLines, int lineHeight)
    {
        g_pRenderText->SetFont(bold ? g_hFontBold : g_hFont);
        const std::vector<std::wstring> lines = WrapTextToWidth(text, width,
            [](const wchar_t* line, size_t length) { return static_cast<int>(g_pRenderText->MeasureText(line, static_cast<int>(length)).cx); });
        const int count = std::min(static_cast<int>(lines.size()), maxLines);
        for (int line = 0; line < count; ++line)
        {
            g_pRenderText->SetTextColor((colour >> 16) & 0xFF, (colour >> 8) & 0xFF, colour & 0xFF, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(y + line * lineHeight), lines[line].c_str(),
                static_cast<float>(width), 0, centred ? RT3_SORT_CENTER : RT3_SORT_LEFT);
        }
        return count;
    }

    constexpr DWORD Rgb(int r, int g, int b)
    {
        return (static_cast<DWORD>(r) << 16) | (static_cast<DWORD>(g) << 8) | static_cast<DWORD>(b);
    }

    constexpr DWORD LABEL_COLOUR = Rgb(205, 210, 220);
    constexpr DWORD MUTED_COLOUR = Rgb(130, 136, 148);
    constexpr DWORD ERROR_COLOUR = Rgb(255, 110, 100);
    constexpr DWORD SUCCESS_COLOUR = Rgb(120, 220, 130);
    constexpr DWORD NOTICE_COLOUR = Rgb(232, 190, 100);
    constexpr DWORD FEE_COLOUR = Rgb(238, 224, 168);

    // 100000 -> "100,000".
    std::wstring GroupedNumber(uint32_t value)
    {
        std::wstring digits = std::to_wstring(value);
        for (int position = static_cast<int>(digits.size()) - 3; position > 0; position -= 3)
        {
            digits.insert(static_cast<size_t>(position), L",");
        }
        return digits;
    }

    std::wstring PercentText(uint16_t basisPoints)
    {
        std::wstring text = std::to_wstring(basisPoints / 100);
        if (basisPoints % 100 != 0)
        {
            text += L"." + std::to_wstring((basisPoints % 100) / 10);
        }
        return text + L"%";
    }
}

CNewUIAuctionSellPanel::CNewUIAuctionSellPanel()
    : m_Pos{ 0, 0 }, m_bActive(false), m_bBuyNowSupported(false), m_bBusy(false), m_bHasItem(false),
      m_bCreateRequested(false), m_bHasResult(false), m_bResultSuccess(false), m_Mode(AuctionHouse::SellMode::Auction),
      m_RowCount(1), m_SaleFee(0), m_ListingFeeBasisPoints(0), m_SuccessFeeBasisPoints(0), m_KindLabels{}, m_KindIndex{},
      m_DurationLabels{}, m_DurationHours{}, m_DurationCount(0)
{
}

void CNewUIAuctionSellPanel::Create(int windowX, int windowY, bool buyNowSupported)
{
    m_Pos = { windowX, windowY };
    m_bBuyNowSupported = buyNowSupported;

    m_KindLabels[0] = L"Zen";
    m_KindLabels[1] = L"Jewel of Chaos";
    m_KindLabels[2] = L"Jewel of Bless";
    m_KindLabels[3] = L"Jewel of Soul";
    m_KindLabels[4] = L"Jewel of Life";
    m_KindLabels[5] = L"Jewel of Creation";
    m_KindLabels[6] = L"Jewel of Guardian";
    m_KindLabels[7] = L"Jewel of Harmony";
    m_KindLabels[8] = L"Fruit of Strength";
    m_KindLabels[9] = L"Fruit of Agility";
    m_KindLabels[10] = L"Fruit of Vitality";
    m_KindLabels[11] = L"Fruit of Energy";
    m_KindLabels[12] = L"Fruit of Command";

    constexpr int MaxVisibleKinds = 6;
    for (int row = 0; row < MaxRows; ++row)
    {
        m_KindIndex[row] = row;
        m_KindCombos[row].Setup(0, 0, BUYNOW_KIND_WIDTH, FIELD_HEIGHT, m_KindLabels, KindCount, row, MaxVisibleKinds);
        m_KindCombos[row].SetFieldSkin(&AuctionFieldSkin());
    }

    const auto initInput = [](CUITextInputBox& input, int maxLength, bool numeric)
    {
        input.Init(g_hWnd, 40, INPUT_HEIGHT, maxLength, false);
        input.SetTextColor(255, 255, 230, 210);
        input.SetBackColor(0, 0, 0, 0);
        input.SetSelectBackColor(255, 95, 68, 24);
        input.SetFont(g_hFont);
        input.SetParentUIID(SEASON3B::INTERFACE_AUCTION_HOUSE);
        if (numeric)
        {
            input.SetOption(UIOPTION_NUMBERONLY);
        }
        input.SetState(UISTATE_HIDE);
    };
    for (auto& input : m_QuantityInputs)
    {
        initInput(input, 10, true);
    }
    for (auto& input : m_BuyoutInputs)
    {
        initInput(input, 10, true);
    }
    initInput(m_NoteInput, NOTE_MAX_CHARACTERS, false);

    SetServerSettings(m_ListingFeeBasisPoints, m_SuccessFeeBasisPoints, 0x0F, 24);
    m_DurationCombo.SetFieldSkin(&AuctionFieldSkin());

    InitButton(m_BtnAdd, 0, 0, AUCTION_KIND_WIDTH, L"+ Add currency");
    InitButton(m_BtnCreate, 0, 0, CREATE_WIDTH, L"Create Listing");
    InitButton(m_BtnClear, 0, 0, CLEAR_WIDTH, L"Clear");

    LayoutControls();
}

void CNewUIAuctionSellPanel::InitButton(CNewUIButton& button, int x, int y, int width, const wchar_t* caption)
{
    button.ChangeText(caption);
    button.ChangeTextBackColor(RGBA(255, 255, 255, 0));
    button.ChangeButtonImgState(true, SEASON3B::CNewUIAuctionWindow::IMAGE_AUCTION_PAGE_BTN, true);
    button.ChangeButtonInfo(x, y, width, BUTTON_HEIGHT);
    button.ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    button.ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
    button.SetNineSliceSkin(&AuctionButtonSkin());
}

void CNewUIAuctionSellPanel::SetPos(int windowX, int windowY)
{
    m_Pos = { windowX, windowY };
    LayoutControls();
}

void CNewUIAuctionSellPanel::LayoutControls()
{
    const bool auction = m_Mode == AuctionHouse::SellMode::Auction;
    const int kindX = auction ? AUCTION_KIND_X : BUYNOW_KIND_X;
    const int kindWidth = auction ? AUCTION_KIND_WIDTH : BUYNOW_KIND_WIDTH;
    const int quantityX = auction ? AUCTION_QUANTITY_X : BUYNOW_QUANTITY_X;
    const int quantityWidth = (auction ? AUCTION_QUANTITY_WIDTH : BUYNOW_QUANTITY_WIDTH) - 2 * INPUT_INSET;
    for (int row = 0; row < MaxRows; ++row)
    {
        const int rowY = m_Pos.y + ROWS_Y + row * ROW_PITCH;
        m_KindCombos[row].Setup(m_Pos.x + kindX, rowY, kindWidth, FIELD_HEIGHT, m_KindLabels, KindCount, m_KindIndex[row], 6);
        m_KindCombos[row].SetFieldSkin(&AuctionFieldSkin());
        m_QuantityInputs[row].SetSize(quantityWidth, INPUT_HEIGHT);
        m_QuantityInputs[row].SetPosition(m_Pos.x + quantityX + INPUT_INSET, rowY + TEXT_Y_OFFSET);
        m_BuyoutInputs[row].SetSize(AUCTION_BUYOUT_WIDTH - 2 * INPUT_INSET, INPUT_HEIGHT);
        m_BuyoutInputs[row].SetPosition(m_Pos.x + AUCTION_BUYOUT_X + INPUT_INSET, rowY + TEXT_Y_OFFSET);
    }

    m_DurationCombo.SetPos(m_Pos.x + DURATION_X, m_Pos.y + OPTIONS_Y);
    m_NoteInput.SetSize(NOTE_WIDTH - 2 * INPUT_INSET, INPUT_HEIGHT);
    m_NoteInput.SetPosition(m_Pos.x + NOTE_X + INPUT_INSET, m_Pos.y + OPTIONS_Y + TEXT_Y_OFFSET);

    m_BtnAdd.ChangeButtonInfo(m_Pos.x + kindX, m_Pos.y + ROWS_Y + m_RowCount * ROW_PITCH, AUCTION_KIND_WIDTH, BUTTON_HEIGHT);
    m_BtnCreate.ChangeButtonInfo(m_Pos.x + WINDOW_WIDTH - BUTTON_MARGIN - CREATE_WIDTH, m_Pos.y + BUTTON_Y, CREATE_WIDTH, BUTTON_HEIGHT);
    m_BtnClear.ChangeButtonInfo(m_Pos.x + BUTTON_MARGIN, m_Pos.y + BUTTON_Y, CLEAR_WIDTH, BUTTON_HEIGHT);
}

void CNewUIAuctionSellPanel::RefreshVisibility()
{
    const bool auction = m_Mode == AuctionHouse::SellMode::Auction;
    for (int row = 0; row < MaxRows; ++row)
    {
        const bool shown = m_bActive && row < m_RowCount;
        m_QuantityInputs[row].SetState(shown ? UISTATE_NORMAL : UISTATE_HIDE);
        m_BuyoutInputs[row].SetState(shown && auction ? UISTATE_NORMAL : UISTATE_HIDE);
    }
    m_NoteInput.SetState(m_bActive ? UISTATE_NORMAL : UISTATE_HIDE);
}

void CNewUIAuctionSellPanel::SetActive(bool active)
{
    m_bActive = active;
    if (!active)
    {
        ReleaseInputFocus();
        for (auto& combo : m_KindCombos)
        {
            combo.Close();
        }
        m_DurationCombo.Close();
    }
    RefreshVisibility();
}

void CNewUIAuctionSellPanel::SetServerSettings(uint16_t listingFeeBasisPoints, uint16_t successFeeBasisPoints,
    uint8_t durationMask, uint8_t defaultDurationHours)
{
    m_ListingFeeBasisPoints = listingFeeBasisPoints;
    m_SuccessFeeBasisPoints = successFeeBasisPoints;

    const uint8_t mask = durationMask == 0 ? 0x0F : durationMask;
    m_DurationCount = 0;
    int defaultIndex = 0;
    for (int choice = 0; choice < DurationChoices; ++choice)
    {
        if ((mask & (1u << choice)) == 0)
        {
            continue;
        }
        if (DurationHoursAll[choice] == defaultDurationHours)
        {
            defaultIndex = m_DurationCount;
        }
        m_DurationLabels[m_DurationCount] = DurationLabelsAll[choice];
        m_DurationHours[m_DurationCount] = DurationHoursAll[choice];
        ++m_DurationCount;
    }
    m_DurationCombo.Setup(m_Pos.x + DURATION_X, m_Pos.y + OPTIONS_Y, DURATION_WIDTH, FIELD_HEIGHT, m_DurationLabels,
        m_DurationCount, defaultIndex);
}

void CNewUIAuctionSellPanel::SetBusy(bool busy)
{
    m_bBusy = busy;
}

void CNewUIAuctionSellPanel::SetMixedSupported(bool mixedSupported)
{
    m_bBuyNowSupported = mixedSupported;
}

void CNewUIAuctionSellPanel::SetSaleFee(uint32_t saleFee)
{
    m_SaleFee = saleFee;
}

void CNewUIAuctionSellPanel::SetItemSelected(bool hasItem)
{
    m_bHasItem = hasItem;
}

void CNewUIAuctionSellPanel::ShowResult(bool success, const std::wstring& message)
{
    m_bHasResult = true;
    m_bResultSuccess = success;
    m_ResultMessage = message;
    if (success)
    {
        ClearForm();
    }
}

void CNewUIAuctionSellPanel::ClearForm()
{
    m_Mode = AuctionHouse::SellMode::Auction;
    m_RowCount = 1;
    for (int row = 0; row < MaxRows; ++row)
    {
        m_KindIndex[row] = row;
        m_KindCombos[row].SetSelectedIndex(row);
        m_QuantityInputs[row].SetText(L"");
        m_BuyoutInputs[row].SetText(L"");
    }
    m_NoteInput.SetText(L"");
    for (int choice = 0; choice < m_DurationCount; ++choice)
    {
        if (m_DurationHours[choice] == DEFAULT_DURATION_HOURS)
        {
            m_DurationCombo.SetSelectedIndex(choice);
        }
    }
    LayoutControls();
    RefreshVisibility();
}

void CNewUIAuctionSellPanel::SetMode(AuctionHouse::SellMode mode)
{
    if (m_Mode == mode)
    {
        return;
    }

    ReleaseInputFocus();
    m_Mode = mode;
    m_bHasResult = false;
    LayoutControls();
    RefreshVisibility();
}

void CNewUIAuctionSellPanel::AddRow()
{
    if (m_RowCount >= MaxRows)
    {
        return;
    }

    // The new row starts on the first kind no other row is using.
    int kind = 0;
    while (kind < KindCount)
    {
        bool used = false;
        for (int row = 0; row < m_RowCount; ++row)
        {
            used = used || m_KindIndex[row] == kind;
        }
        if (!used)
        {
            break;
        }
        ++kind;
    }
    m_KindIndex[m_RowCount] = kind;
    m_KindCombos[m_RowCount].SetSelectedIndex(kind);
    m_QuantityInputs[m_RowCount].SetText(L"");
    m_BuyoutInputs[m_RowCount].SetText(L"");
    ++m_RowCount;
    LayoutControls();
    RefreshVisibility();
}

void CNewUIAuctionSellPanel::RemoveRow(int row)
{
    if (m_RowCount <= 1 || row < 0 || row >= m_RowCount)
    {
        return;
    }

    ReleaseInputFocus();
    for (int i = row; i + 1 < m_RowCount; ++i)
    {
        wchar_t text[16]{};
        m_QuantityInputs[i + 1].GetText(text, static_cast<int>(std::size(text)));
        m_QuantityInputs[i].SetText(text);
        wchar_t buyout[16]{};
        m_BuyoutInputs[i + 1].GetText(buyout, static_cast<int>(std::size(buyout)));
        m_BuyoutInputs[i].SetText(buyout);
        m_KindIndex[i] = m_KindIndex[i + 1];
        m_KindCombos[i].SetSelectedIndex(m_KindIndex[i]);
    }
    m_QuantityInputs[m_RowCount - 1].SetText(L"");
    m_BuyoutInputs[m_RowCount - 1].SetText(L"");
    --m_RowCount;
    LayoutControls();
    RefreshVisibility();
}

void CNewUIAuctionSellPanel::KeepKindsUnique(int changedRow)
{
    // Picking a currency another row already has swaps the two, so a price can never name a kind twice.
    const int chosen = m_KindCombos[changedRow].GetSelectedIndex();
    const int previous = m_KindIndex[changedRow];
    for (int row = 0; row < m_RowCount; ++row)
    {
        if (row != changedRow && m_KindIndex[row] == chosen)
        {
            m_KindIndex[row] = previous;
            m_KindCombos[row].SetSelectedIndex(previous);
        }
    }
    m_KindIndex[changedRow] = chosen;
}

int64_t CNewUIAuctionSellPanel::ParseQuantity(CUITextInputBox& input) const
{
    wchar_t text[16]{};
    input.GetText(text, static_cast<int>(std::size(text)));
    if (text[0] == L'\0')
    {
        return 0;
    }

    wchar_t* end = nullptr;
    const unsigned long long parsed = std::wcstoull(text, &end, 10);
    // The wire carries 32-bit amounts; anything larger is not a usable price.
    return (*end != L'\0' || parsed > UINT32_MAX) ? 0 : static_cast<int64_t>(parsed);
}

int CNewUIAuctionSellPanel::SelectedDurationHours() const
{
    const int index = m_DurationCombo.GetSelectedIndex();
    return index >= 0 && index < m_DurationCount ? m_DurationHours[index] : 24;
}

AuctionHouse::SellDraft CNewUIAuctionSellPanel::BuildDraft()
{
    AuctionHouse::SellDraft draft;
    draft.mode = m_Mode;
    draft.durationHours = static_cast<uint8_t>(SelectedDurationHours());
    const bool auction = m_Mode == AuctionHouse::SellMode::Auction;
    for (int row = 0; row < m_RowCount; ++row)
    {
        draft.rows.push_back({ static_cast<AuctionHouse::PriceKind>(m_KindCombos[row].GetSelectedIndex()),
            ParseQuantity(m_QuantityInputs[row]), auction ? ParseQuantity(m_BuyoutInputs[row]) : 0 });
    }
    return draft;
}

bool CNewUIAuctionSellPanel::AnyInputFocused() const
{
    for (int row = 0; row < MaxRows; ++row)
    {
        if (const_cast<CUITextInputBox&>(m_QuantityInputs[row]).HaveFocus()
            || const_cast<CUITextInputBox&>(m_BuyoutInputs[row]).HaveFocus())
        {
            return true;
        }
    }
    return const_cast<CUITextInputBox&>(m_NoteInput).HaveFocus();
}

bool CNewUIAuctionSellPanel::HasInputFocus() const
{
    return AnyInputFocused();
}

HWND CNewUIAuctionSellPanel::FocusedInputHandle() const
{
    for (int row = 0; row < MaxRows; ++row)
    {
        if (const_cast<CUITextInputBox&>(m_QuantityInputs[row]).HaveFocus())
        {
            return const_cast<CUITextInputBox&>(m_QuantityInputs[row]).GetHandle();
        }
        if (const_cast<CUITextInputBox&>(m_BuyoutInputs[row]).HaveFocus())
        {
            return const_cast<CUITextInputBox&>(m_BuyoutInputs[row]).GetHandle();
        }
    }
    if (const_cast<CUITextInputBox&>(m_NoteInput).HaveFocus())
    {
        return const_cast<CUITextInputBox&>(m_NoteInput).GetHandle();
    }
    return nullptr;
}

void CNewUIAuctionSellPanel::ReleaseInputFocus()
{
    if (AnyInputFocused())
    {
        CUITextInputBox::ReleaseFocus();
    }
}

bool CNewUIAuctionSellPanel::IsMouseOverAnyField() const
{
    const bool auction = m_Mode == AuctionHouse::SellMode::Auction;
    for (int row = 0; row < m_RowCount; ++row)
    {
        const int y = m_Pos.y + ROWS_Y + row * ROW_PITCH;
        const int x = m_Pos.x + (auction ? AUCTION_QUANTITY_X : BUYNOW_QUANTITY_X);
        const int width = auction ? AUCTION_QUANTITY_WIDTH : BUYNOW_QUANTITY_WIDTH;
        if (SEASON3B::CheckMouseIn(x, y, width, FIELD_HEIGHT)
            || (auction && SEASON3B::CheckMouseIn(m_Pos.x + AUCTION_BUYOUT_X, y, AUCTION_BUYOUT_WIDTH, FIELD_HEIGHT)))
        {
            return true;
        }
    }
    return SEASON3B::CheckMouseIn(m_Pos.x + NOTE_X, m_Pos.y + OPTIONS_Y, NOTE_WIDTH, FIELD_HEIGHT);
}

bool CNewUIAuctionSellPanel::UpdateMouseEvent()
{
    if (!m_bActive)
    {
        return false;
    }

    const bool auction = m_Mode == AuctionHouse::SellMode::Auction;
    bool handled = false;

    if (AnyInputFocused() && IsPress(VK_LBUTTON) && !IsMouseOverAnyField())
    {
        CUITextInputBox::ReleaseFocus();
    }

    // Mode buttons.
    if (IsRelease(VK_LBUTTON))
    {
        for (int mode = 0; mode < 2; ++mode)
        {
            const int x = m_Pos.x + FORM_X + mode * (MODE_WIDTH + MODE_GAP);
            if (SEASON3B::CheckMouseIn(x, m_Pos.y + MODE_Y, MODE_WIDTH, MODE_HEIGHT))
            {
                SetMode(mode == 0 ? AuctionHouse::SellMode::Auction : AuctionHouse::SellMode::BuyNow);
                handled = true;
            }
        }
    }

    // Combos. A dropdown that is open owns the mouse until it closes; otherwise every visible combo sees it.
    const int liveRows = m_RowCount;
    int openKind = -1;
    for (int row = 0; row < liveRows; ++row)
    {
        if (m_KindCombos[row].IsOpen())
        {
            openKind = row;
        }
    }
    const bool durationOpen = m_DurationCombo.IsOpen();
    if (openKind >= 0)
    {
        if (m_KindCombos[openKind].UpdateMouseEvent())
        {
            KeepKindsUnique(openKind);
        }
        handled = true;
    }
    else if (durationOpen)
    {
        m_DurationCombo.UpdateMouseEvent();
        handled = true;
    }
    else
    {
        for (int row = 0; row < liveRows; ++row)
        {
            if (m_KindCombos[row].UpdateMouseEvent())
            {
                KeepKindsUnique(row);
            }
            handled = handled || m_KindCombos[row].IsMouseOverWidget();
        }
        m_DurationCombo.UpdateMouseEvent();
        handled = handled || m_DurationCombo.IsMouseOverWidget();
    }

    // Text inputs: a press anywhere on a field's frame focuses its input.
    const auto handleInput = [&](CUITextInputBox& input, int fieldX, int fieldY, int fieldWidth)
    {
        input.DoAction();
        if (IsPress(VK_LBUTTON) && !input.HaveFocus() && SEASON3B::CheckMouseIn(fieldX, fieldY, fieldWidth, FIELD_HEIGHT))
        {
            input.GiveFocus();
        }
        handled = handled || SEASON3B::CheckMouseIn(fieldX, fieldY, fieldWidth, FIELD_HEIGHT);
    };
    if (openKind < 0 && !durationOpen)
    {
        for (int row = 0; row < liveRows; ++row)
        {
            handleInput(m_QuantityInputs[row], m_Pos.x + (auction ? AUCTION_QUANTITY_X : BUYNOW_QUANTITY_X),
                m_Pos.y + ROWS_Y + row * ROW_PITCH, auction ? AUCTION_QUANTITY_WIDTH : BUYNOW_QUANTITY_WIDTH);
            if (auction)
            {
                handleInput(m_BuyoutInputs[row], m_Pos.x + AUCTION_BUYOUT_X, m_Pos.y + ROWS_Y + row * ROW_PITCH, AUCTION_BUYOUT_WIDTH);
            }
        }
        handleInput(m_NoteInput, m_Pos.x + NOTE_X, m_Pos.y + OPTIONS_Y, NOTE_WIDTH);

        // Price rows: remove boxes and the add button, in both modes.
        {
            if (m_RowCount < MaxRows && m_BtnAdd.UpdateMouseEvent())
            {
                AddRow();
                PlayBuffer(SOUND_CLICK01);
                handled = true;
            }
            if (m_RowCount > 1 && IsRelease(VK_LBUTTON))
            {
                for (int row = 0; row < m_RowCount; ++row)
                {
                    if (SEASON3B::CheckMouseIn(m_Pos.x + (auction ? AUCTION_REMOVE_X : BUYNOW_REMOVE_X), m_Pos.y + ROWS_Y + row * ROW_PITCH,
                            BUYNOW_REMOVE_SIZE, BUYNOW_REMOVE_SIZE))
                    {
                        RemoveRow(row);
                        PlayBuffer(SOUND_CLICK01);
                        handled = true;
                        break;
                    }
                }
            }
        }

        if (m_BtnClear.UpdateMouseEvent())
        {
            ClearForm();
            m_bHasResult = false;
            PlayBuffer(SOUND_CLICK01);
            handled = true;
        }
        if (m_BtnCreate.UpdateMouseEvent())
        {
            RequestCreate();
            PlayBuffer(SOUND_CLICK01);
            handled = true;
        }
    }

    return handled;
}

void CNewUIAuctionSellPanel::RequestCreate()
{
    if (!m_bHasItem || m_bBusy)
    {
        return;
    }

    if (AuctionHouse::ValidateSellDraft(BuildDraft(), m_bBuyNowSupported) == AuctionHouse::SellDraftIssue::None)
    {
        m_bCreateRequested = true;
        m_bHasResult = false;
    }
}

bool CNewUIAuctionSellPanel::TakeCreateRequest(AuctionHouse::SellDraft& draft, std::wstring& note)
{
    if (!m_bCreateRequested)
    {
        return false;
    }

    m_bCreateRequested = false;
    draft = BuildDraft();
    wchar_t text[NOTE_MAX_CHARACTERS + 8]{};
    m_NoteInput.GetText(text, static_cast<int>(std::size(text)));
    note = text;
    return true;
}

std::wstring CNewUIAuctionSellPanel::StatusText(AuctionHouse::SellDraftIssue issue) const
{
    if (m_bHasResult)
    {
        return m_ResultMessage;
    }

    if (!m_bHasItem)
    {
        return I18N::Game::AuctionSelectItem;
    }

    switch (issue)
    {
    case AuctionHouse::SellDraftIssue::NoPrice: return L"Enter a price.";
    case AuctionHouse::SellDraftIssue::ZeroQuantity: return L"Every currency needs an amount above zero.";
    case AuctionHouse::SellDraftIssue::TooManyComponents: return L"A price can mix at most five currencies.";
    case AuctionHouse::SellDraftIssue::DuplicateKind: return L"Each currency can appear only once.";
    case AuctionHouse::SellDraftIssue::BuyoutBelowStart: return L"A buyout cannot be below its starting amount.";
    case AuctionHouse::SellDraftIssue::BuyoutIncomplete: return L"Set a buyout for every currency, or leave them all empty.";
    case AuctionHouse::SellDraftIssue::MixedUnavailable:
    case AuctionHouse::SellDraftIssue::BuyNowUnavailable: return L"Mixed prices are not available on this server.";
    default: return std::wstring();
    }
}

void CNewUIAuctionSellPanel::RenderField(int x, int y, int width, bool focused) const
{
    const UI::NineSlice::StateTints& tints = AuctionFieldSkin().tints;
    UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(width), static_cast<float>(FIELD_HEIGHT), focused ? tints.pressed : tints.normal);
}

void CNewUIAuctionSellPanel::GetItemIconRect(int& x, int& y, int& size) const
{
    x = m_Pos.x + ITEM_SLOT_X + ITEM_ICON_INSET;
    y = m_Pos.y + ITEM_SLOT_Y + ITEM_ICON_INSET;
    size = ITEM_SLOT_SIZE - 2 * ITEM_ICON_INSET;
}

bool CNewUIAuctionSellPanel::IsMouseOverItemIcon() const
{
    int x = 0;
    int y = 0;
    int size = 0;
    GetItemIconRect(x, y, size);
    return m_bHasItem && SEASON3B::CheckMouseIn(x, y, size, size);
}

void CNewUIAuctionSellPanel::RenderItemCard(const ITEM* item)
{
    UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(m_Pos.x + RAIL_X),
        static_cast<float>(m_Pos.y + ITEM_CARD_Y), static_cast<float>(RAIL_WIDTH), static_cast<float>(ITEM_CARD_HEIGHT),
        CARD_TINT);
    RenderAuctionItemSlot(m_Pos.x + ITEM_SLOT_X, m_Pos.y + ITEM_SLOT_Y, ITEM_SLOT_SIZE,
        GetAuctionItemSlotTint(item, IsMouseOverItemIcon()));

    const int textX = m_Pos.x + RAIL_X + RAIL_TEXT_PAD;
    const int textWidth = RAIL_WIDTH - 2 * RAIL_TEXT_PAD;
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    if (item == nullptr)
    {
        RenderWrapped(I18N::Game::AuctionSelectItem, textX, m_Pos.y + ITEM_NAME_Y, textWidth, NOTICE_COLOUR, true, true,
            ITEM_NAME_MAX_LINES, TEXT_LINE_HEIGHT + 1);
        RenderWrapped(I18N::Game::AuctionSelectItemHint, textX, m_Pos.y + ITEM_HINT_Y, textWidth, MUTED_COLOUR, false, true,
            ITEM_HINT_MAX_LINES, TEXT_LINE_HEIGHT);
        return;
    }

    // The name takes the item's rarity colour, which RenderWrapped cannot take as a plain RGB, so wrap it here.
    g_pRenderText->SetFont(g_hFontBold);
    const std::wstring name = GetItemDisplayName(const_cast<ITEM*>(item));
    const std::vector<std::wstring> lines = WrapTextToWidth(name, textWidth,
        [](const wchar_t* line, size_t length) { return static_cast<int>(g_pRenderText->MeasureText(line, static_cast<int>(length)).cx); });
    const int lineCount = std::min(static_cast<int>(lines.size()), ITEM_NAME_MAX_LINES);
    for (int line = 0; line < lineCount; ++line)
    {
        SetBrowseItemNameColor(item);
        g_pRenderText->RenderText(static_cast<float>(textX), static_cast<float>(m_Pos.y + ITEM_NAME_Y + line * (TEXT_LINE_HEIGHT + 1)),
            lines[line].c_str(), static_cast<float>(textWidth), 0, RT3_SORT_CENTER);
    }
    RenderWrapped(I18N::Game::AuctionSelectItemHint, textX, m_Pos.y + ITEM_HINT_Y, textWidth, MUTED_COLOUR, false, true,
        ITEM_HINT_MAX_LINES, TEXT_LINE_HEIGHT);
}

void CNewUIAuctionSellPanel::RenderModeButtons()
{
    for (int mode = 0; mode < 2; ++mode)
    {
        const bool selected = (mode == 0) == (m_Mode == AuctionHouse::SellMode::Auction);
        const int x = m_Pos.x + FORM_X + mode * (MODE_WIDTH + MODE_GAP);
        const bool hovered = !selected && SEASON3B::CheckMouseIn(x, m_Pos.y + MODE_Y, MODE_WIDTH, MODE_HEIGHT);
        UI::NineSlice::Render(AuctionButtonSkin().texture, static_cast<float>(x), static_cast<float>(m_Pos.y + MODE_Y),
            static_cast<float>(MODE_WIDTH), static_cast<float>(MODE_HEIGHT),
            selected ? MODE_TINT_SELECTED : (hovered ? MODE_TINT_HOVER : MODE_TINT_IDLE));

        const wchar_t* label = mode == 0 ? L"Auction" : L"Buy Now";
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(selected ? 255 : 205, selected ? 215 : 205, selected ? 100 : 205, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 0);
        const SIZE labelSize = g_pRenderText->MeasureText(label, static_cast<int>(wcslen(label)));
        g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(m_Pos.y + MODE_Y + (MODE_HEIGHT - labelSize.cy) / 2),
            label, static_cast<float>(MODE_WIDTH), 0, RT3_SORT_CENTER);
    }
}

void CNewUIAuctionSellPanel::RenderPricePanel()
{
    const bool auction = m_Mode == AuctionHouse::SellMode::Auction;
    UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(m_Pos.x + FORM_X),
        static_cast<float>(m_Pos.y + PRICE_PANEL_Y), static_cast<float>(FORM_WIDTH),
        static_cast<float>(PRICE_PANEL_HEIGHT), CARD_TINT);

    // Mixed prices say so up front when the server cannot take them yet, instead of letting the player fill them in first.
    if (!auction && !m_bBuyNowSupported)
    {
        RenderLabel(L"Buy Now listings are not available on this server yet.", m_Pos.x + PRICE_LABEL_X,
            m_Pos.y + PRICE_DESCRIPTION_Y, FORM_WIDTH - 16, NOTICE_COLOUR, true);
    }
    else
    {
        RenderLabel(auction ? L"Bids keep every currency and raise at least one. A buyout is optional."
                            : L"A fixed price in up to five currencies, bought at once. No bidding.",
            m_Pos.x + PRICE_LABEL_X, m_Pos.y + PRICE_DESCRIPTION_Y, FORM_WIDTH - 16, MUTED_COLOUR, false);
    }

    const int kindX = auction ? AUCTION_KIND_X : BUYNOW_KIND_X;
    const int quantityX = auction ? AUCTION_QUANTITY_X : BUYNOW_QUANTITY_X;
    RenderLabel(L"Currency", m_Pos.x + kindX, m_Pos.y + ROW_HEADER_Y, 100, MUTED_COLOUR, true);
    RenderLabel(auction ? L"Starting amount" : L"Amount", m_Pos.x + quantityX, m_Pos.y + ROW_HEADER_Y, 110, MUTED_COLOUR, true);
    if (auction)
    {
        RenderLabel(L"Buyout (optional)", m_Pos.x + AUCTION_BUYOUT_X, m_Pos.y + ROW_HEADER_Y, 118, MUTED_COLOUR, true);
    }

    for (int row = 0; row < m_RowCount; ++row)
    {
        const int rowY = m_Pos.y + ROWS_Y + row * ROW_PITCH;
        RenderField(m_Pos.x + quantityX, rowY, auction ? AUCTION_QUANTITY_WIDTH : BUYNOW_QUANTITY_WIDTH,
            m_QuantityInputs[row].HaveFocus());
        m_QuantityInputs[row].Render();
        if (auction)
        {
            RenderField(m_Pos.x + AUCTION_BUYOUT_X, rowY, AUCTION_BUYOUT_WIDTH, m_BuyoutInputs[row].HaveFocus());
            m_BuyoutInputs[row].Render();
        }
        if (m_RowCount > 1)
        {
            const int x = m_Pos.x + (auction ? AUCTION_REMOVE_X : BUYNOW_REMOVE_X);
            const bool hovered = SEASON3B::CheckMouseIn(x, rowY, BUYNOW_REMOVE_SIZE, BUYNOW_REMOVE_SIZE);
            UI::NineSlice::Render(AuctionButtonSkin().texture, static_cast<float>(x), static_cast<float>(rowY),
                static_cast<float>(BUYNOW_REMOVE_SIZE), static_cast<float>(BUYNOW_REMOVE_SIZE),
                hovered ? MODE_TINT_HOVER : REMOVE_TINT);
            g_pRenderText->SetFont(g_hFontBold);
            g_pRenderText->SetTextColor(255, 255, 255, 255);
            g_pRenderText->SetBgColor(0, 0, 0, 0);
            g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(rowY + TEXT_Y_OFFSET), L"x",
                static_cast<float>(BUYNOW_REMOVE_SIZE), 0, RT3_SORT_CENTER);
        }
    }

    if (m_RowCount < MaxRows)
    {
        m_BtnAdd.Render();
    }
}

void CNewUIAuctionSellPanel::RenderOptions()
{
    UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(m_Pos.x + FORM_X),
        static_cast<float>(m_Pos.y + OPTIONS_CARD_Y), static_cast<float>(FORM_WIDTH),
        static_cast<float>(OPTIONS_CARD_HEIGHT), CARD_TINT);
    RenderLabel(L"Duration", m_Pos.x + DURATION_LABEL_X, m_Pos.y + OPTIONS_Y + TEXT_Y_OFFSET, 52, LABEL_COLOUR, true);
    RenderLabel(L"Note", m_Pos.x + NOTE_LABEL_X, m_Pos.y + OPTIONS_Y + TEXT_Y_OFFSET, 28, LABEL_COLOUR, true);
    RenderField(m_Pos.x + NOTE_X, m_Pos.y + OPTIONS_Y, NOTE_WIDTH, m_NoteInput.HaveFocus());
    m_NoteInput.Render();

    wchar_t text[40]{};
    m_NoteInput.GetText(text, static_cast<int>(std::size(text)));
    if (text[0] == L'\0' && !m_NoteInput.HaveFocus())
    {
        RenderLabel(L"Optional message for buyers", m_Pos.x + NOTE_X + INPUT_INSET, m_Pos.y + OPTIONS_Y + TEXT_Y_OFFSET,
            NOTE_WIDTH - 2 * INPUT_INSET, MUTED_COLOUR, false);
    }
}

void CNewUIAuctionSellPanel::RenderFees(const AuctionHouse::SellDraft& draft)
{
    UI::NineSlice::Render(AuctionFieldSkin().texture, static_cast<float>(m_Pos.x + RAIL_X),
        static_cast<float>(m_Pos.y + FEE_CARD_Y), static_cast<float>(RAIL_WIDTH), static_cast<float>(FEE_CARD_HEIGHT),
        CARD_TINT);

    const int textX = m_Pos.x + RAIL_X + RAIL_TEXT_PAD;
    const int textWidth = RAIL_WIDTH - 2 * RAIL_TEXT_PAD;
    RenderLabel(L"Listing tax", textX, m_Pos.y + FEE_TITLE_Y, textWidth, LABEL_COLOUR, true);

    // The only tax: a flat amount held when the listing is created, returned if the item does not sell.
    if (m_SaleFee > 0)
    {
        RenderLabel((GroupedNumber(m_SaleFee) + L" Zen").c_str(), textX, m_Pos.y + FEE_LISTING_LABEL_Y, textWidth, FEE_COLOUR, true);
        RenderWrapped(L"Held now. Returned to your Mailbox if it does not sell; kept when it sells. No other fees.", textX,
            m_Pos.y + FEE_LISTING_LABEL_Y + TEXT_LINE_HEIGHT + 3, textWidth, MUTED_COLOUR, false, false, 6, TEXT_LINE_HEIGHT);
        return;
    }

    // Older servers: a listing fee taken now and a success fee taken from the proceeds.
    RenderLabel(L"Listing fee (now)", textX, m_Pos.y + FEE_LISTING_LABEL_Y, textWidth, MUTED_COLOUR, false);
    const auto fields = AuctionHouse::ToCreateFields(draft);
    std::wstring listing = L"-";
    if (fields && !fields->starting.IsZero())
    {
        const AuctionHouse::AuctionAmount listingFee = AuctionHouse::ComputeAuctionFee(fields->starting, m_ListingFeeBasisPoints);
        listing = PriceSummaryText(fields->currencyMode, listingFee);
    }
    RenderWrapped(listing, textX, m_Pos.y + FEE_LISTING_LABEL_Y + TEXT_LINE_HEIGHT, textWidth, FEE_COLOUR, false, false, 2,
        TEXT_LINE_HEIGHT);

    RenderLabel(L"Success fee", textX, m_Pos.y + FEE_SUCCESS_LABEL_Y, textWidth, MUTED_COLOUR, false);
    RenderWrapped(PercentText(m_SuccessFeeBasisPoints) + L" of the final price when it sells", textX,
        m_Pos.y + FEE_SUCCESS_LABEL_Y + TEXT_LINE_HEIGHT, textWidth, FEE_COLOUR, false, false, 3, TEXT_LINE_HEIGHT);
}

void CNewUIAuctionSellPanel::Render(const ITEM* item)
{
    m_bHasItem = item != nullptr;
    EnableAlphaTest();
    RenderItemCard(item);
    RenderModeButtons();
    RenderPricePanel();
    RenderOptions();

    const AuctionHouse::SellDraft draft = BuildDraft();
    const AuctionHouse::SellDraftIssue issue = AuctionHouse::ValidateSellDraft(draft, m_bBuyNowSupported);
    RenderFees(draft);

    const std::wstring status = StatusText(issue);
    if (!status.empty())
    {
        RenderLabel(status.c_str(), m_Pos.x + FORM_X + 4, m_Pos.y + STATUS_Y, FORM_WIDTH - 8,
            m_bHasResult ? (m_bResultSuccess ? SUCCESS_COLOUR : ERROR_COLOUR) : ERROR_COLOUR, false);
    }

    const bool canCreate = m_bHasItem && !m_bBusy && issue == AuctionHouse::SellDraftIssue::None;
    if (canCreate)
    {
        m_BtnCreate.UnLock();
        m_BtnCreate.ChangeTextColor(RGBA(255, 255, 255, 255));
    }
    else
    {
        m_BtnCreate.Lock();
        m_BtnCreate.ChangeTextColor(RGBA(100, 100, 100, 255));
    }
    m_BtnCreate.Render();
    m_BtnClear.Render();

    // Dropdowns last, lower rows first, so an open list sits over everything beneath it.
    m_DurationCombo.Render();
    const int liveRows = m_RowCount;
    for (int row = liveRows - 1; row >= 0; --row)
    {
        m_KindCombos[row].Render();
    }
}
