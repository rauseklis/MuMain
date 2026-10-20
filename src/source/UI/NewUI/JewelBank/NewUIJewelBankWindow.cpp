// NewUIJewelBankWindow.cpp: the Jewel Bank window, opened with Ctrl+J.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/JewelBank/NewUIJewelBankWindow.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/AuctionHouse/AuctionModel.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/NineSlice.h"
#include "Audio/DSPlaySound.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/SocketSystem.h"
#include "UI/Scaling/UITransform.h"
#include "I18N/All.h"

#include <algorithm>

namespace
{
    // The frame is the Auction House's: its textures are loaded for good when the Auction window is created at startup,
    // so this window borrows them instead of loading a second copy.
    using Auction = SEASON3B::CNewUIAuctionWindow;

    constexpr int TOP_BAND_HEIGHT = 64;
    constexpr int BOTTOM_BAND_HEIGHT = 45;
    constexpr int SIDE_BAND_WIDTH = 21;
    constexpr int FRAME_TEXTURE_WIDTH = 190;
    constexpr int FRAME_CAP_WIDTH = 64;
    constexpr int FRAME_CENTER_WIDTH = FRAME_TEXTURE_WIDTH - 2 * FRAME_CAP_WIDTH;
    constexpr int CLOSE_BTN_WIDTH = 36;
    constexpr int CLOSE_BTN_HEIGHT = 29;
    constexpr int CLOSE_BTN_MARGIN = 8;
    constexpr int TITLE_BAND_HEIGHT = 40;

    constexpr int LIST_X = 22;
    constexpr int LIST_Y = 50;
    constexpr int LIST_WIDTH = SEASON3B::CNewUIJewelBankWindow::WINDOW_WIDTH - 2 * LIST_X;
    constexpr int ROW_HEIGHT = 20;
    constexpr int ROW_GAP = 2;
    constexpr int MARKER_SIZE = 12;

    constexpr int PANEL_Y = LIST_Y + JewelBank::KIND_COUNT * (ROW_HEIGHT + ROW_GAP) + 6;
    constexpr int AMOUNT_LABEL_Y = PANEL_Y + 6;
    constexpr int AMOUNT_FIELD_X = LIST_X + 64;
    constexpr int AMOUNT_FIELD_WIDTH = 96;
    constexpr int AMOUNT_FIELD_HEIGHT = 23;
    constexpr int AMOUNT_FIELD_TEXT_INSET = 6;
    constexpr int BUTTON_HEIGHT = 20;
    constexpr int BUTTON_ROW_1_Y = PANEL_Y + 34;
    constexpr int BUTTON_ROW_2_Y = BUTTON_ROW_1_Y + BUTTON_HEIGHT + 6;
    constexpr int BUTTON_WIDTH = 96;
    constexpr int CHECKBOX_Y = BUTTON_ROW_2_Y + BUTTON_HEIGHT + 10;
    constexpr int MESSAGE_Y = CHECKBOX_Y + 22;
    constexpr int AMOUNT_MAX_DIGITS = 6;

    const DWORD ROW_TINT_IDLE = RGBA(68, 74, 86, 255);
    const DWORD ROW_TINT_HOVER = RGBA(160, 140, 90, 255);
    const DWORD ROW_TINT_SELECTED = RGBA(224, 192, 108, 255);

    int RowY(int index)
    {
        return LIST_Y + index * (ROW_HEIGHT + ROW_GAP);
    }
} // namespace

SEASON3B::CNewUIJewelBankWindow::CNewUIJewelBankWindow()
    : m_pNewUIMng(nullptr), m_Pos{ 0, 0 }, m_bDragging(false), m_iDragGrabOffsetX(0), m_iDragGrabOffsetY(0), m_Counts{},
      m_bHasState(false), m_bAutoDeposit(false), m_iSelected(-1), m_bOperationPending(false), m_PendingOperationId{},
      m_PendingOperation(JewelBank::Operation::Deposit), m_bMessageGood(true)
{
}

SEASON3B::CNewUIJewelBankWindow::~CNewUIJewelBankWindow()
{
    Release();
}

bool SEASON3B::CNewUIJewelBankWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (nullptr == pNewUIMng)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_JEWEL_BANK, this);
    m_Pos.x = x;
    m_Pos.y = y;

    m_BtnClose.ChangeButtonImgState(true, Auction::IMAGE_AUCTION_CLOSE_BTN, false);
    m_BtnClose.ChangeToolTipText(&I18N::Game::Close388);

    InitButton(&m_BtnWithdraw, 0, 0, BUTTON_WIDTH, L"Withdraw");
    InitButton(&m_BtnMax, 0, 0, BUTTON_WIDTH, L"Max");
    InitButton(&m_BtnDeposit, 0, 0, BUTTON_WIDTH, L"Deposit");
    InitButton(&m_BtnDepositAll, 0, 0, BUTTON_WIDTH, L"Deposit All");

    // The input sits in a recessed field drawn behind it, so it paints no background of its own.
    m_AmountInput.Init(g_hWnd, AMOUNT_FIELD_WIDTH - 2 * AMOUNT_FIELD_TEXT_INSET, AMOUNT_FIELD_HEIGHT, AMOUNT_MAX_DIGITS, false);
    m_AmountInput.SetTextColor(255, 255, 230, 210);
    m_AmountInput.SetBackColor(0, 0, 0, 0);
    m_AmountInput.SetSelectBackColor(255, 95, 68, 24);
    m_AmountInput.SetFont(g_hFont);
    m_AmountInput.SetParentUIID(SEASON3B::INTERFACE_JEWEL_BANK);
    m_AmountInput.SetOption(UIOPTION_NUMBERONLY);
    m_AmountInput.SetState(UISTATE_HIDE);

    PositionChildren();
    RefreshButtons();
    Show(false);
    return true;
}

void SEASON3B::CNewUIJewelBankWindow::Release()
{
    m_AmountInput.SetState(UISTATE_HIDE);
    if (m_AmountInput.HaveFocus())
    {
        CUITextInputBox::ReleaseFocus();
    }

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIJewelBankWindow::InitButton(CNewUIButton* button, int x, int y, int width, const wchar_t* caption)
{
    button->ChangeText(caption);
    button->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    button->ChangeButtonImgState(true, Auction::IMAGE_AUCTION_PAGE_BTN, true);
    button->ChangeButtonInfo(x, y, width, BUTTON_HEIGHT);
    button->ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    button->ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
    button->SetNineSliceSkin(&UI::AuctionChrome::AuctionButtonSkin());
}

void SEASON3B::CNewUIJewelBankWindow::PositionChildren()
{
    m_BtnClose.ChangeButtonInfo(m_Pos.x + WINDOW_WIDTH - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN, m_Pos.y + CLOSE_BTN_MARGIN, CLOSE_BTN_WIDTH, CLOSE_BTN_HEIGHT);
    const int rightX = m_Pos.x + WINDOW_WIDTH - LIST_X - BUTTON_WIDTH;
    m_BtnMax.ChangeButtonInfo(m_Pos.x + AMOUNT_FIELD_X + AMOUNT_FIELD_WIDTH + 8, m_Pos.y + AMOUNT_LABEL_Y - 1, 52, BUTTON_HEIGHT);
    m_BtnWithdraw.ChangeButtonInfo(m_Pos.x + LIST_X, m_Pos.y + BUTTON_ROW_1_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_BtnDeposit.ChangeButtonInfo(rightX, m_Pos.y + BUTTON_ROW_1_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_BtnDepositAll.ChangeButtonInfo(rightX, m_Pos.y + BUTTON_ROW_2_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_AmountInput.SetPosition(m_Pos.x + AMOUNT_FIELD_X + AMOUNT_FIELD_TEXT_INSET, m_Pos.y + AMOUNT_LABEL_Y);
}

void SEASON3B::CNewUIJewelBankWindow::OpeningProcess()
{
    SetLayoutMode(UI::Scaling::LayoutMode::Dialog);
    m_bDragging = false;
    m_bOperationPending = false;
    m_Message.clear();
    m_iSelected = -1;
    m_AmountInput.SetText(L"");
    m_AmountInput.SetState(UISTATE_NORMAL);
    PositionChildren();
    RefreshButtons();
    SetRelatedWnd(g_hWnd);
    SendOpenRequest();
}

void SEASON3B::CNewUIJewelBankWindow::ClosingProcess()
{
    m_bDragging = false;
    m_AmountInput.SetState(UISTATE_HIDE);
    if (m_AmountInput.HaveFocus())
    {
        CUITextInputBox::ReleaseFocus();
    }
    SetRelatedWnd(g_hWnd);
}

void SEASON3B::CNewUIJewelBankWindow::SetState(const JewelBank::StateResponse& state)
{
    if (state.Result != JewelBank::Result::Success)
    {
        ShowMessage(JewelBank::ResultText(state.Result), false);
        return;
    }

    m_Counts = state.Counts;
    m_bAutoDeposit = state.AutoDeposit;
    m_bHasState = true;
    RefreshButtons();
}

void SEASON3B::CNewUIJewelBankWindow::SetOperationResponse(const JewelBank::OperationResponse& response)
{
    if (!m_bOperationPending || response.OperationId != m_PendingOperationId)
    {
        return;
    }

    m_bOperationPending = false;
    if (response.Result != JewelBank::Result::Success)
    {
        ShowMessage(JewelBank::ResultText(response.Result), false);
    }
    else if (response.Operation == JewelBank::Operation::Withdraw)
    {
        const JewelBank::KindInfo* kind = JewelBank::FindKind(response.KindCode);
        ShowMessage(L"Withdrew " + JewelBank::FormatCount(response.Units) + L" " + (kind ? kind->Name : L"items") + L".", true);
    }
    else if (response.Operation == JewelBank::Operation::Deposit)
    {
        ShowMessage(response.Units > 0 ? L"Deposited " + JewelBank::FormatCount(response.Units) + L" items." : std::wstring(L"Deposited."), true);
    }
    else
    {
        m_Message.clear();
    }
    RefreshButtons();
}

void SEASON3B::CNewUIJewelBankWindow::SetPickupNotification(const JewelBank::PickupNotification& notification)
{
    const std::wstring text = JewelBank::PickupMessage(notification);
    if (!text.empty())
    {
        g_pSystemLogBox->AddText(text.c_str(), SEASON3B::TYPE_SYSTEM_MESSAGE);
    }

    if (const JewelBank::KindInfo* kind = JewelBank::FindKind(notification.KindCode))
    {
        m_Counts[kind->Code - JewelBank::FIRST_KIND_CODE] = notification.NewCount;
    }
}

void SEASON3B::CNewUIJewelBankWindow::ShowMessage(const std::wstring& text, bool good)
{
    m_Message = text;
    m_bMessageGood = good;
}

uint64_t SEASON3B::CNewUIJewelBankWindow::SelectedCount() const
{
    return m_iSelected >= 0 && m_iSelected < JewelBank::KIND_COUNT ? m_Counts[m_iSelected] : 0;
}

void SEASON3B::CNewUIJewelBankWindow::RefreshButtons()
{
    const auto setEnabled = [](CNewUIButton& button, bool enabled)
    {
        if (enabled)
        {
            button.UnLock();
        }
        else
        {
            button.Lock();
        }
        const DWORD tint = enabled ? RGBA(255, 255, 255, 255) : RGBA(100, 100, 100, 255);
        button.ChangeImgColor(BUTTON_STATE_UP, tint);
        button.ChangeTextColor(tint);
    };

    const bool idle = !m_bOperationPending && m_bHasState;
    setEnabled(m_BtnWithdraw, idle && SelectedCount() > 0);
    setEnabled(m_BtnMax, idle && SelectedCount() > 0);
    setEnabled(m_BtnDeposit, idle && m_iSelected >= 0);
    setEnabled(m_BtnDepositAll, idle);
}

void SEASON3B::CNewUIJewelBankWindow::BeginOperation(JewelBank::Operation operation)
{
    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_PendingOperation = operation;
    m_bOperationPending = true;
    m_Message.clear();
    RefreshButtons();
}

void SEASON3B::CNewUIJewelBankWindow::SendOpenRequest()
{
    SocketClient->ToGameServer()->SendJewelBankOpenRequest();
}

void SEASON3B::CNewUIJewelBankWindow::SendDeposit(uint8_t kindCode)
{
    BeginOperation(JewelBank::Operation::Deposit);
    SocketClient->ToGameServer()->SendJewelBankDepositRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()), kindCode);
}

void SEASON3B::CNewUIJewelBankWindow::SendWithdraw()
{
    wchar_t text[16]{};
    m_AmountInput.GetText(text, static_cast<int>(std::size(text)));
    const uint32_t units = JewelBank::WithdrawAmount(JewelBank::ParseAmount(text), SelectedCount());
    if (units == 0)
    {
        ShowMessage(L"Enter how many to withdraw.", false);
        return;
    }

    BeginOperation(JewelBank::Operation::Withdraw);
    SocketClient->ToGameServer()->SendJewelBankWithdrawRequest(
        m_PendingOperationId.data(), static_cast<uint32_t>(m_PendingOperationId.size()),
        JewelBank::KindAt(m_iSelected).Code, units);
}

void SEASON3B::CNewUIJewelBankWindow::SendAutoDeposit(bool enabled)
{
    SocketClient->ToGameServer()->SendJewelBankSetAutoDepositRequest(enabled ? 1 : 0);
}

bool SEASON3B::CNewUIJewelBankWindow::RowAt(int mouseX, int mouseY, int& index) const
{
    if (mouseX < m_Pos.x + LIST_X || mouseX >= m_Pos.x + LIST_X + LIST_WIDTH)
    {
        return false;
    }

    for (int i = 0; i < JewelBank::KIND_COUNT; ++i)
    {
        const int top = m_Pos.y + RowY(i);
        if (mouseY >= top && mouseY < top + ROW_HEIGHT)
        {
            index = i;
            return true;
        }
    }
    return false;
}

bool SEASON3B::CNewUIJewelBankWindow::BtnProcess()
{
    const int closeX = m_Pos.x + WINDOW_WIDTH - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN;
    const int closeY = m_Pos.y + CLOSE_BTN_MARGIN;
    if (CheckMouseIn(closeX, closeY, CLOSE_BTN_WIDTH, CLOSE_BTN_HEIGHT) && IsRelease(VK_LBUTTON))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_JEWEL_BANK);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnMax.UpdateMouseEvent() == true)
    {
        const uint32_t units = JewelBank::WithdrawAmount(UINT32_MAX, SelectedCount());
        m_AmountInput.SetText(std::to_wstring(units).c_str());
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnWithdraw.UpdateMouseEvent() == true)
    {
        SendWithdraw();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnDeposit.UpdateMouseEvent() == true)
    {
        if (m_iSelected >= 0)
        {
            SendDeposit(JewelBank::KindAt(m_iSelected).Code);
        }
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnDepositAll.UpdateMouseEvent() == true)
    {
        SendDeposit(0);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIJewelBankWindow::UpdateMouseEvent()
{
    if (BtnProcess())
    {
        return false;
    }

    m_AmountInput.DoAction();
    if (m_AmountInput.HaveFocus())
    {
        SetRelatedWnd(m_AmountInput.GetHandle());
    }

    if (m_bDragging)
    {
        if (!MouseLButton)
        {
            m_bDragging = false;
            return false;
        }

        m_Pos.x = MouseX - m_iDragGrabOffsetX;
        m_Pos.y = MouseY - m_iDragGrabOffsetY;
        PositionChildren();
        return false;
    }

    const bool overTitleBar = CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH - CLOSE_BTN_WIDTH - CLOSE_BTN_MARGIN, TITLE_BAND_HEIGHT);
    if (overTitleBar && IsPress(VK_LBUTTON))
    {
        m_bDragging = true;
        m_iDragGrabOffsetX = MouseX - m_Pos.x;
        m_iDragGrabOffsetY = MouseY - m_Pos.y;
        return false;
    }

    int row = 0;
    if (IsRelease(VK_LBUTTON) && RowAt(MouseX, MouseY, row))
    {
        m_iSelected = row;
        RefreshButtons();
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    // The auto-deposit tick: the box and its label are one click target.
    const int boxX = m_Pos.x + LIST_X;
    const int boxY = m_Pos.y + CHECKBOX_Y;
    if (!m_bOperationPending && IsRelease(VK_LBUTTON) && CheckMouseIn(boxX, boxY, LIST_WIDTH, 18))
    {
        SendAutoDeposit(!m_bAutoDeposit);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    if (CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH, WINDOW_HEIGHT))
    {
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIJewelBankWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_JEWEL_BANK) != true)
    {
        return true;
    }

    if (m_AmountInput.HaveFocus())
    {
        if (SEASON3B::IsPress(VK_RETURN) == true)
        {
            SendWithdraw();
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

    if (SEASON3B::IsPress(VK_ESCAPE) == true)
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_JEWEL_BANK);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIJewelBankWindow::Update()
{
    return true;
}

float SEASON3B::CNewUIJewelBankWindow::GetLayerDepth()
{
    // The same band as the Auction House and the other standalone account windows.
    return 10.1f;
}

bool SEASON3B::CNewUIJewelBankWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

bool SEASON3B::CNewUIJewelBankWindow::Render()
{
    EnableAlphaTest();

    RenderFrame();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->RenderText((float)(m_Pos.x + 15), (float)(m_Pos.y + 13), L"Jewel Bank", (float)(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);

    RenderRows();
    RenderActionPanel();
    m_BtnClose.Render();

    EnableAlphaTest();
    return true;
}

void SEASON3B::CNewUIJewelBankWindow::RenderFrame()
{
    const float x = static_cast<float>(m_Pos.x);
    const float y = static_cast<float>(m_Pos.y);
    const float centerWidth = static_cast<float>(WINDOW_WIDTH - 2 * FRAME_CAP_WIDTH);

    // The same construction as the Auction House frame: the stone background stretched over the window, the two ornate
    // caps of each band at 1:1, and only the plain centre strip stretched.
    RenderImageStretch(Auction::IMAGE_AUCTION_BACK, x, y, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT),
        0.f, 0.f, static_cast<float>(Auction::WINDOW_WIDTH), static_cast<float>(Auction::WINDOW_HEIGHT));

    RenderImageStretch(Auction::IMAGE_AUCTION_TOP, x, y, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT),
        0.f, 0.f, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT));
    RenderImageStretch(Auction::IMAGE_AUCTION_TOP, x + FRAME_CAP_WIDTH, y, centerWidth, static_cast<float>(TOP_BAND_HEIGHT),
        static_cast<float>(FRAME_CAP_WIDTH), 0.f, static_cast<float>(FRAME_CENTER_WIDTH), static_cast<float>(TOP_BAND_HEIGHT));
    RenderImageStretch(Auction::IMAGE_AUCTION_TOP, x + WINDOW_WIDTH - FRAME_CAP_WIDTH, y,
        static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT),
        static_cast<float>(FRAME_TEXTURE_WIDTH - FRAME_CAP_WIDTH), 0.f, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(TOP_BAND_HEIGHT));

    const float sideHeight = static_cast<float>(WINDOW_HEIGHT - TOP_BAND_HEIGHT - BOTTOM_BAND_HEIGHT);
    RenderImageStretch(Auction::IMAGE_AUCTION_LEFT, x, y + TOP_BAND_HEIGHT, static_cast<float>(SIDE_BAND_WIDTH), sideHeight,
        0.f, 0.f, static_cast<float>(SIDE_BAND_WIDTH), sideHeight);
    RenderImageStretch(Auction::IMAGE_AUCTION_RIGHT, x + WINDOW_WIDTH - SIDE_BAND_WIDTH, y + TOP_BAND_HEIGHT,
        static_cast<float>(SIDE_BAND_WIDTH), sideHeight, 0.f, 0.f, static_cast<float>(SIDE_BAND_WIDTH), sideHeight);

    const float bottomY = y + WINDOW_HEIGHT - BOTTOM_BAND_HEIGHT;
    RenderImageStretch(Auction::IMAGE_AUCTION_BOTTOM, x, bottomY, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT),
        0.f, 0.f, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT));
    RenderImageStretch(Auction::IMAGE_AUCTION_BOTTOM, x + FRAME_CAP_WIDTH, bottomY, centerWidth, static_cast<float>(BOTTOM_BAND_HEIGHT),
        static_cast<float>(FRAME_CAP_WIDTH), 0.f, static_cast<float>(FRAME_CENTER_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT));
    RenderImageStretch(Auction::IMAGE_AUCTION_BOTTOM, x + WINDOW_WIDTH - FRAME_CAP_WIDTH, bottomY,
        static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT),
        static_cast<float>(FRAME_TEXTURE_WIDTH - FRAME_CAP_WIDTH), 0.f, static_cast<float>(FRAME_CAP_WIDTH), static_cast<float>(BOTTOM_BAND_HEIGHT));
}

void SEASON3B::CNewUIJewelBankWindow::RenderRows()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    int hovered = -1;
    RowAt(MouseX, MouseY, hovered);
    for (int i = 0; i < JewelBank::KIND_COUNT; ++i)
    {
        const JewelBank::KindInfo& kind = JewelBank::KindAt(i);
        const int rowX = m_Pos.x + LIST_X;
        const int rowY = m_Pos.y + RowY(i);
        const bool selected = i == m_iSelected;
        UI::NineSlice::Render(UI::AuctionChrome::AuctionRowSkin().texture, static_cast<float>(rowX), static_cast<float>(rowY),
            static_cast<float>(LIST_WIDTH), static_cast<float>(ROW_HEIGHT),
            selected ? ROW_TINT_SELECTED : (i == hovered ? ROW_TINT_HOVER : ROW_TINT_IDLE));

        // A small coloured marker in front of the name stands in for the item icon.
        UI::AuctionChrome::RenderAuctionItemSlot(rowX + 6, rowY + (ROW_HEIGHT - MARKER_SIZE) / 2, MARKER_SIZE, kind.Color);

        g_pRenderText->SetTextColor(230, 230, 230, 255);
        g_pRenderText->RenderText(static_cast<float>(rowX + 24), static_cast<float>(rowY + 5), kind.Name, 0, 0, RT3_SORT_LEFT);

        const uint64_t count = m_Counts[i];
        if (count > 0)
        {
            g_pRenderText->SetTextColor(255, 220, 130, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(130, 130, 130, 255);
        }
        g_pRenderText->RenderText(static_cast<float>(rowX), static_cast<float>(rowY + 5), JewelBank::FormatCount(count).c_str(),
            static_cast<float>(LIST_WIDTH - 10), 0, RT3_SORT_RIGHT);
    }
}

void SEASON3B::CNewUIJewelBankWindow::RenderActionPanel()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    // The amount: a caption, a recessed field behind the input (a gold ring while it has focus), and Max.
    g_pRenderText->SetTextColor(190, 190, 190, 255);
    g_pRenderText->RenderText(static_cast<float>(m_Pos.x + LIST_X), static_cast<float>(m_Pos.y + AMOUNT_LABEL_Y + 6), L"Amount", 0, 0, RT3_SORT_LEFT);
    const UI::NineSlice::StateTints& tints = UI::AuctionChrome::AuctionFieldSkin().tints;
    UI::NineSlice::Render(UI::AuctionChrome::AuctionFieldSkin().texture, static_cast<float>(m_Pos.x + AMOUNT_FIELD_X),
        static_cast<float>(m_Pos.y + AMOUNT_LABEL_Y - 2), static_cast<float>(AMOUNT_FIELD_WIDTH), static_cast<float>(AMOUNT_FIELD_HEIGHT),
        m_AmountInput.HaveFocus() ? tints.pressed : tints.normal);
    m_AmountInput.Render();

    m_BtnMax.Render();
    m_BtnWithdraw.Render();
    m_BtnDeposit.Render();
    m_BtnDepositAll.Render();

    // The auto-deposit tick.
    RenderCheckBox(m_Pos.x + LIST_X, m_Pos.y + CHECKBOX_Y, m_bAutoDeposit ? TRUE : FALSE);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(220, 220, 200, 255);
    g_pRenderText->RenderText(static_cast<float>(m_Pos.x + LIST_X + 22), static_cast<float>(m_Pos.y + CHECKBOX_Y + 3),
        L"Send picked-up jewels and fruits to the bank", 0, 0, RT3_SORT_LEFT);

    if (!m_Message.empty())
    {
        if (m_bMessageGood)
        {
            g_pRenderText->SetTextColor(150, 230, 150, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(255, 130, 110, 255);
        }
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y + MESSAGE_Y), m_Message.c_str(),
            static_cast<float>(WINDOW_WIDTH), 0, RT3_SORT_CENTER);
    }
    else if (!m_bHasState)
    {
        g_pRenderText->SetTextColor(170, 170, 170, 255);
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y + MESSAGE_Y), L"Loading...",
            static_cast<float>(WINDOW_WIDTH), 0, RT3_SORT_CENTER);
    }
}
