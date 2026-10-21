// NewUIJewelBankWindow.cpp: the Jewel Bank window, opened with Ctrl+J.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/JewelBank/NewUIJewelBankWindow.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/AuctionHouse/AuctionModel.h"
#include "UI/NewUI/AuctionHouse/NewUIAuctionWindow.h"
#include "UI/NewUI/Inventory/NewUIStorageInventory.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/NineSlice.h"
#include "Audio/DSPlaySound.h"
#include "Network/Server/WSclient.h"
#include "Network/Server/SocketSystem.h"
#include "UI/Scaling/UITransform.h"

#include <algorithm>
#include <cmath>

namespace
{
    // The frame is the Auction House's: its textures are loaded for good when the Auction window is created at startup,
    // so this window borrows them instead of loading a second copy. The round close "X" is part of that frame's top
    // right corner; the shared HandleFrameCornerClose gives it its click.
    using Auction = SEASON3B::CNewUIAuctionWindow;
    using Window = SEASON3B::CNewUIJewelBankWindow;

    constexpr int TOP_BAND_HEIGHT = 64;
    constexpr int BOTTOM_BAND_HEIGHT = 45;
    constexpr int SIDE_BAND_WIDTH = 21;
    constexpr int FRAME_TEXTURE_WIDTH = 190;
    constexpr int FRAME_CAP_WIDTH = 64;
    constexpr int FRAME_CENTER_WIDTH = FRAME_TEXTURE_WIDTH - 2 * FRAME_CAP_WIDTH;
    constexpr int TITLE_BAND_HEIGHT = 30;
    constexpr int CORNER_CLOSE_RESERVE = 34;

    // Everything below the frame's title bar starts under the soft shadow line the frame art draws there (it ends at y 36).
    constexpr int TAB_Y = 41;
    constexpr int TAB_WIDTH = 72;
    constexpr int TAB_HEIGHT = 18;
    constexpr int TAB_GAP = 3;

    constexpr int HEADER_Y = 62;
    constexpr int LIST_X = 22;
    constexpr int LIST_Y = 74;
    constexpr int LIST_WIDTH = 316;
    // Every page lists fifteen rows of the same size.
    constexpr int ROW_HEIGHT = 16;
    constexpr int ROW_GAP = 1;

    // The pager under the list: an arrow, the page name, an arrow, centred in the window. The arrows are the storage window's
    // hand-made HD arrow buttons (20 x 14 logical pixels, 4x art), drawn a little larger.
    constexpr int PAGER_ARROW_WIDTH = 28;
    constexpr int PAGER_LABEL_WIDTH = 120;
    constexpr int PAGER_GAP = 6;
    constexpr int PAGER_WIDTH = 2 * PAGER_ARROW_WIDTH + PAGER_LABEL_WIDTH + 2 * PAGER_GAP;
    constexpr int PAGER_HEIGHT = 20;
    constexpr int PAGER_Y = LIST_Y + JewelBank::ROW_COUNT * (ROW_HEIGHT + ROW_GAP) + 3;

    // Columns, relative to the left edge of a row.
    // A row is selected by clicking it, so it has no tick box. The icon is a little smaller than the row, so the model
    // stays inside it; the amount cell is flush with the row's right edge.
    constexpr int CHECK_X = 6;
    constexpr int CHECK_SIZE = 11;
    constexpr int ICON_X = 6;
    constexpr int ICON_SIZE = 14;
    constexpr int NAME_X = 28;
    constexpr int BANK_RIGHT = 170;
    constexpr int BANK_WIDTH = 40;
    constexpr int INVENTORY_RIGHT = 232;
    constexpr int INVENTORY_WIDTH = 60;
    constexpr int AMOUNT_WIDTH = 64;
    constexpr int AMOUNT_X = LIST_WIDTH - AMOUNT_WIDTH;
    constexpr int AMOUNT_INSET = 4;
    constexpr int AMOUNT_TEXT_Y = 2;

    constexpr int BUTTON_HEIGHT = 20;
    constexpr int BUTTON_GAP = 6;
    constexpr int BUTTON_WIDTH = (LIST_WIDTH - 2 * BUTTON_GAP) / 3;
    constexpr int BUTTON_ROW_Y = PAGER_Y + PAGER_HEIGHT + 3;
    constexpr int AUTO_Y = BUTTON_ROW_Y + BUTTON_HEIGHT + 5;
    constexpr int MESSAGE_Y = AUTO_Y + 17;
    constexpr int AMOUNT_MAX_DIGITS = 6;

    constexpr auto POLL_INTERVAL = std::chrono::milliseconds(1500);
    constexpr auto OPERATION_TIMEOUT = std::chrono::seconds(6);

    const DWORD ROW_TINT_IDLE = RGBA(68, 74, 86, 255);
    const DWORD ROW_TINT_HOVER = RGBA(120, 112, 84, 255);
    const DWORD ROW_TINT_SELECTED = RGBA(224, 192, 108, 255);
    const DWORD TAB_TINT_IDLE = RGBA(172, 178, 190, 255);
    const DWORD TAB_TINT_HOVER = RGBA(236, 241, 250, 255);
    const DWORD TAB_TINT_ACTIVE = RGBA(232, 198, 112, 255);

    // The top of a row, from the window's top.
    int RowY(int row)
    {
        return LIST_Y + row * (ROW_HEIGHT + ROW_GAP);
    }

    // The left edges of the pager's two arrows, from the window's left.
    int PagerLeftX()
    {
        return (Window::WINDOW_WIDTH - PAGER_WIDTH) / 2;
    }

    int PagerRightX()
    {
        return PagerLeftX() + PAGER_ARROW_WIDTH + PAGER_GAP + PAGER_LABEL_WIDTH + PAGER_GAP;
    }

    // The two tabs are centred in the window.
    int TabX(int tab)
    {
        const int total = Window::TAB_COUNT * TAB_WIDTH + (Window::TAB_COUNT - 1) * TAB_GAP;
        return (Window::WINDOW_WIDTH - total) / 2 + tab * (TAB_WIDTH + TAB_GAP);
    }

    // The item model each kind is shown with: the jewel's own item, or the fruit item at the fruit type's level.
    struct KindIcon
    {
        int type;
        int level;
        // How much of the slot the model fills. The models are not authored to one size, so this evens them out.
        float fill;
    };

    KindIcon IconOf(int index)
    {
        // The fill which makes each jewel look as big as the others; a bundle of a jewel uses the same fill as the jewel,
        // so a jewel keeps its size on every page.
        static const KindIcon icons[JewelBank::ROW_COUNT] = {
            { ITEM_JEWEL_OF_CHAOS, 0, 0.80f }, { ITEM_JEWEL_OF_BLESS, 0, 0.84f }, { ITEM_JEWEL_OF_SOUL, 0, 0.70f },
            { ITEM_JEWEL_OF_LIFE, 0, 0.80f }, { ITEM_JEWEL_OF_CREATION, 0, 0.74f }, { ITEM_JEWEL_OF_GUARDIAN, 0, 0.80f },
            { ITEM_JEWEL_OF_HARMONY, 0, 0.80f }, { ITEM_GEMSTONE, 0, 0.80f },
            { ITEM_LOWER_REFINE_STONE, 0, 0.80f }, { ITEM_HIGHER_REFINE_STONE, 0, 0.80f },
            { ITEM_FRUITS, 0, 0.96f }, { ITEM_FRUITS, 1, 0.96f }, { ITEM_FRUITS, 2, 0.96f }, { ITEM_FRUITS, 3, 0.96f }, { ITEM_FRUITS, 4, 0.96f },
        };

        // The bundles are the jewels' packed items, in the order of the jewels (the same ten, once per size, level 0 / 1 / 2).
        static const int packed[JewelBank::BUNDLES_PER_SIZE] = {
            ITEM_PACKED_JEWEL_OF_CHAOS, ITEM_PACKED_JEWEL_OF_BLESS, ITEM_PACKED_JEWEL_OF_SOUL, ITEM_PACKED_JEWEL_OF_LIFE,
            ITEM_PACKED_JEWEL_OF_CREATION, ITEM_PACKED_JEWEL_OF_GUARDIAN, ITEM_PACKED_JEWEL_OF_HARMONY, ITEM_PACKED_GEMSTONE,
            ITEM_PACKED_LOWER_REFINE_STONE, ITEM_PACKED_HIGHER_REFINE_STONE,
        };
        if (index >= JewelBank::FIRST_BUNDLE_INDEX)
        {
            const int bundle = std::clamp(index, JewelBank::FIRST_BUNDLE_INDEX, JewelBank::KIND_COUNT - 1) - JewelBank::FIRST_BUNDLE_INDEX;
            const int jewel = bundle % JewelBank::BUNDLES_PER_SIZE;
            return { packed[jewel], bundle / JewelBank::BUNDLES_PER_SIZE, icons[jewel].fill };
        }

        return icons[std::clamp(index, 0, JewelBank::ROW_COUNT - 1)];
    }

    // A filled circle drawn from half-pixel slices (the flat-colour helper only draws rectangles).
    void FillCircle(float centerX, float centerY, float radius, unsigned int argb)
    {
        constexpr float STEP = 0.5f;
        for (float y = -radius; y < radius; y += STEP)
        {
            const float mid = y + STEP * 0.5f;
            const float half = std::sqrt(std::max(0.f, radius * radius - mid * mid));
            RenderColorQuadARGB(centerX - half, centerY + y, half * 2.f, STEP, argb);
        }
    }

    void SetTextColorArgb(DWORD argb)
    {
        g_pRenderText->SetTextColor((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, argb >> 24);
    }
} // namespace

SEASON3B::CNewUIJewelBankWindow::CNewUIJewelBankWindow()
    : m_pNewUIMng(nullptr), m_pNewUI3DRenderMng(nullptr), m_Pos{ 0, 0 }, m_bDragging(false), m_iDragGrabOffsetX(0), m_iDragGrabOffsetY(0), m_iEditRow(-1),
      m_iTab(TAB_DEPOSIT), m_iPage(0), m_Amounts{}, m_Counts{}, m_Inventory{}, m_bHasState(false), m_bAutoDeposit(false),
      m_bOperationPending(false), m_PendingOperationId{}, m_PendingOperation(JewelBank::Operation::Deposit), m_PendingRequested(0),
      m_OperationDeadline{}, m_NextPoll{}, m_bMessageGood(true)
{
}

SEASON3B::CNewUIJewelBankWindow::~CNewUIJewelBankWindow()
{
    Release();
}

bool SEASON3B::CNewUIJewelBankWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y)
{
    if (nullptr == pNewUIMng || nullptr == pNewUI3DRenderMng)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_JEWEL_BANK, this);
    m_pNewUI3DRenderMng = pNewUI3DRenderMng;
    m_pNewUI3DRenderMng->Add3DRenderObj(this, INFORMATION_CAMERA_Z_ORDER);
    m_Pos.x = x;
    m_Pos.y = y;

    InitButton(&m_BtnSelectAll, L"Select all");
    InitButton(&m_BtnClear, L"Clear");
    InitButton(&m_BtnAction, L"Deposit");

    // The input sits over the amount cell of the row being edited, which draws its own recessed field behind it.
    m_AmountInput.Init(g_hWnd, AMOUNT_WIDTH - 2 * AMOUNT_INSET, ROW_HEIGHT, AMOUNT_MAX_DIGITS, false);
    m_AmountInput.SetTextColor(255, 255, 230, 210);
    m_AmountInput.SetBackColor(0, 0, 0, 0);
    m_AmountInput.SetSelectBackColor(255, 120, 96, 40);
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

void SEASON3B::CNewUIJewelBankWindow::InitButton(CNewUIButton* button, const wchar_t* caption)
{
    button->ChangeText(caption);
    button->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    button->ChangeButtonImgState(true, Auction::IMAGE_AUCTION_PAGE_BTN, true);
    button->ChangeButtonInfo(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
    button->ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    button->ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
    button->SetNineSliceSkin(&UI::AuctionChrome::AuctionButtonSkin());
}

void SEASON3B::CNewUIJewelBankWindow::PositionChildren()
{
    m_BtnSelectAll.ChangeButtonInfo(m_Pos.x + LIST_X, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_BtnClear.ChangeButtonInfo(m_Pos.x + LIST_X + BUTTON_WIDTH + BUTTON_GAP, m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_BtnAction.ChangeButtonInfo(m_Pos.x + LIST_X + 2 * (BUTTON_WIDTH + BUTTON_GAP), m_Pos.y + BUTTON_ROW_Y, BUTTON_WIDTH, BUTTON_HEIGHT);
    if (m_iEditRow >= 0)
    {
        m_AmountInput.SetPosition(m_Pos.x + LIST_X + AMOUNT_X + AMOUNT_INSET, m_Pos.y + RowY(JewelBank::RowOnPage(m_iEditRow)) + AMOUNT_TEXT_Y);
    }
}

void SEASON3B::CNewUIJewelBankWindow::OpeningProcess()
{
    SetLayoutMode(UI::Scaling::LayoutMode::Dialog);
    m_bDragging = false;
    m_bOperationPending = false;
    m_Message.clear();
    m_iEditRow = -1;
    m_AmountInput.SetState(UISTATE_HIDE);
    m_Amounts = {};
    PositionChildren();
    m_iPage = 0;
    SwitchTab(TAB_DEPOSIT);
    SetRelatedWnd(g_hWnd);
    SendOpenRequest();
    m_NextPoll = Clock::now() + POLL_INTERVAL;
}

void SEASON3B::CNewUIJewelBankWindow::ClosingProcess()
{
    m_bDragging = false;
    CancelEdit();
    SetRelatedWnd(g_hWnd);
}

JewelBank::Available SEASON3B::CNewUIJewelBankWindow::AvailableFor(int tab) const
{
    JewelBank::Available available{};
    for (size_t i = 0; i < available.size(); ++i)
    {
        available[i] = tab == TAB_DEPOSIT ? static_cast<uint64_t>(m_Inventory[i]) : m_Counts[i];
    }
    return available;
}

JewelBank::Available SEASON3B::CNewUIJewelBankWindow::CurrentAvailable() const
{
    return AvailableFor(m_iTab);
}

JewelBank::Amounts& SEASON3B::CNewUIJewelBankWindow::CurrentAmounts()
{
    return m_Amounts[m_iTab];
}

void SEASON3B::CNewUIJewelBankWindow::SwitchTab(int tab)
{
    CancelEdit();
    m_iTab = tab;
    m_Message.clear();
    m_BtnAction.ChangeText(tab == TAB_DEPOSIT ? L"Deposit" : L"Withdraw");
    RefreshButtons();
}

void SEASON3B::CNewUIJewelBankWindow::SwitchPage(int page)
{
    CancelEdit();
    m_iPage = std::clamp(page, 0, JewelBank::PAGE_COUNT - 1);
    m_Message.clear();
}

void SEASON3B::CNewUIJewelBankWindow::SetState(const JewelBank::StateResponse& state)
{
    if (state.Result != JewelBank::Result::Success)
    {
        ShowMessage(JewelBank::ResultText(state.Result), false);
        return;
    }

    m_Counts = state.Counts;
    m_Inventory = state.Inventory;
    m_bAutoDeposit = state.AutoDeposit;
    m_bHasState = true;
    for (int tab = 0; tab < TAB_COUNT; ++tab)
    {
        JewelBank::LimitToAvailable(m_Amounts[tab], AvailableFor(tab));
    }
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
        ShowMessage(L"Withdrew " + JewelBank::FormatCount(response.Units) + L".", true);
        m_Amounts[TAB_WITHDRAW] = {};
    }
    else if (response.Operation == JewelBank::Operation::Deposit)
    {
        std::wstring text = L"Deposited " + JewelBank::FormatCount(response.Units);
        if (response.Units < m_PendingRequested)
        {
            // Less than asked for was carried, or fitted into the bank, by the time the server looked.
            text += L" of " + JewelBank::FormatCount(m_PendingRequested);
        }
        ShowMessage(text + L".", true);
        m_Amounts[TAB_DEPOSIT] = {};
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
    const JewelBank::Available available = CurrentAvailable();
    const bool anyAvailable = std::any_of(available.begin(), available.end(), [](uint64_t units) { return units > 0; });
    setEnabled(m_BtnSelectAll, idle && anyAvailable);
    setEnabled(m_BtnClear, idle && JewelBank::SelectedKinds(CurrentAmounts()) > 0);
    setEnabled(m_BtnAction, idle && JewelBank::SelectedKinds(CurrentAmounts()) > 0);
}

void SEASON3B::CNewUIJewelBankWindow::BeginOperation(JewelBank::Operation operation)
{
    m_PendingOperationId = AuctionHouse::GenerateAuctionOperationId();
    m_PendingOperation = operation;
    m_bOperationPending = true;
    m_OperationDeadline = Clock::now() + OPERATION_TIMEOUT;
    m_Message.clear();
    RefreshButtons();
}

void SEASON3B::CNewUIJewelBankWindow::SendOpenRequest()
{
    SocketClient->ToGameServer()->SendJewelBankOpenRequest();
}

void SEASON3B::CNewUIJewelBankWindow::SendAction()
{
    CommitEdit();
    const JewelBank::Amounts& a = CurrentAmounts();
    const uint64_t total = JewelBank::TotalAmount(a);
    if (total == 0)
    {
        ShowMessage(m_iTab == TAB_DEPOSIT ? L"Click what you want to deposit." : L"Click what you want to withdraw.", false);
        return;
    }

    if (m_iTab == TAB_WITHDRAW && total > JewelBank::MAX_WITHDRAW_UNITS)
    {
        ShowMessage(L"Withdraw at most " + JewelBank::FormatCount(JewelBank::MAX_WITHDRAW_UNITS) + L" at a time.", false);
        return;
    }

    m_PendingRequested = total;
    BeginOperation(m_iTab == TAB_DEPOSIT ? JewelBank::Operation::Deposit : JewelBank::Operation::Withdraw);
    const uint8_t* id = m_PendingOperationId.data();
    const uint32_t idLength = static_cast<uint32_t>(m_PendingOperationId.size());
    // The amounts travel as one little-endian 32-bit value per kind, in the order of the kind codes.
    uint8_t amounts[JewelBank::KIND_COUNT * sizeof(uint32_t)]{};
    JewelBank::AppendRequestAmounts(a, amounts);
    if (m_iTab == TAB_DEPOSIT)
    {
        SocketClient->ToGameServer()->SendJewelBankDepositRequest(id, idLength, amounts, static_cast<uint32_t>(sizeof(amounts)));
    }
    else
    {
        SocketClient->ToGameServer()->SendJewelBankWithdrawRequest(id, idLength, amounts, static_cast<uint32_t>(sizeof(amounts)));
    }
}

void SEASON3B::CNewUIJewelBankWindow::SendAutoDeposit(bool enabled)
{
    SocketClient->ToGameServer()->SendJewelBankSetAutoDepositRequest(enabled ? 1 : 0);
}

void SEASON3B::CNewUIJewelBankWindow::BeginEdit(int row)
{
    CommitEdit();
    if (row < 0 || row >= JewelBank::KIND_COUNT || CurrentAvailable()[row] == 0)
    {
        return;
    }

    m_iEditRow = row;
    const uint32_t current = CurrentAmounts()[row];
    m_AmountInput.SetText(current > 0 ? std::to_wstring(current).c_str() : L"");
    m_AmountInput.SetPosition(m_Pos.x + LIST_X + AMOUNT_X + AMOUNT_INSET, m_Pos.y + RowY(JewelBank::RowOnPage(row)) + AMOUNT_TEXT_Y);
    m_AmountInput.SetState(UISTATE_NORMAL);
    m_AmountInput.GiveFocus(TRUE);
    SetRelatedWnd(m_AmountInput.GetHandle());
}

void SEASON3B::CNewUIJewelBankWindow::CommitEdit()
{
    if (m_iEditRow < 0)
    {
        return;
    }

    wchar_t text[16]{};
    m_AmountInput.GetText(text, static_cast<int>(std::size(text)));
    CurrentAmounts()[m_iEditRow] = JewelBank::ClampAmount(JewelBank::ParseAmount(text), CurrentAvailable()[m_iEditRow]);
    CancelEdit();
    RefreshButtons();
}

void SEASON3B::CNewUIJewelBankWindow::CancelEdit()
{
    m_iEditRow = -1;
    m_AmountInput.SetState(UISTATE_HIDE);
    if (m_AmountInput.HaveFocus())
    {
        CUITextInputBox::ReleaseFocus();
    }
    SetRelatedWnd(g_hWnd);
}

bool SEASON3B::CNewUIJewelBankWindow::RowAt(int mouseX, int mouseY, int& row, bool& onAmount) const
{
    if (mouseX < m_Pos.x + LIST_X || mouseX >= m_Pos.x + LIST_X + LIST_WIDTH)
    {
        return false;
    }

    for (int r = 0; r < JewelBank::ROW_COUNT; ++r)
    {
        const int top = m_Pos.y + RowY(r);
        if (mouseY >= top && mouseY < top + ROW_HEIGHT)
        {
            row = JewelBank::KindOnPage(m_iPage, r);
            onAmount = mouseX >= m_Pos.x + LIST_X + AMOUNT_X;
            return true;
        }
    }
    return false;
}

bool SEASON3B::CNewUIJewelBankWindow::BtnProcess()
{
    // The frame's own round "X" in the top right corner closes the window. The shared helper expects the 190 pixel wide
    // frame the glyph is drawn on, which here is the right-hand end of the wider frame.
    const POINT cornerFrame{ m_Pos.x + WINDOW_WIDTH - FRAME_TEXTURE_WIDTH, m_Pos.y };
    if (g_pNewUISystem->HandleFrameCornerClose(cornerFrame, SEASON3B::INTERFACE_JEWEL_BANK))
    {
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnSelectAll.UpdateMouseEvent() == true)
    {
        CommitEdit();
        CurrentAmounts() = JewelBank::SelectAll(CurrentAvailable());
        RefreshButtons();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnClear.UpdateMouseEvent() == true)
    {
        CommitEdit();
        CurrentAmounts() = {};
        RefreshButtons();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_BtnAction.UpdateMouseEvent() == true)
    {
        SendAction();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIJewelBankWindow::UpdateMouseEvent()
{
    // A click outside the amount cell being edited accepts what was typed.
    if (m_iEditRow >= 0 && IsPress(VK_LBUTTON)
        && !CheckMouseIn(m_Pos.x + LIST_X + AMOUNT_X, m_Pos.y + RowY(JewelBank::RowOnPage(m_iEditRow)), AMOUNT_WIDTH, ROW_HEIGHT))
    {
        CommitEdit();
    }

    if (BtnProcess())
    {
        return false;
    }

    if (m_iEditRow >= 0)
    {
        m_AmountInput.DoAction();
        if (m_AmountInput.HaveFocus())
        {
            SetRelatedWnd(m_AmountInput.GetHandle());
        }
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

    const bool overTitleBar = CheckMouseIn(m_Pos.x, m_Pos.y, WINDOW_WIDTH - CORNER_CLOSE_RESERVE, TITLE_BAND_HEIGHT);
    if (overTitleBar && IsPress(VK_LBUTTON))
    {
        m_bDragging = true;
        m_iDragGrabOffsetX = MouseX - m_Pos.x;
        m_iDragGrabOffsetY = MouseY - m_Pos.y;
        return false;
    }

    if (IsRelease(VK_LBUTTON))
    {
        for (int tab = 0; tab < TAB_COUNT; ++tab)
        {
            if (tab != m_iTab && CheckMouseIn(m_Pos.x + TabX(tab), m_Pos.y + TAB_Y, TAB_WIDTH, TAB_HEIGHT))
            {
                SwitchTab(tab);
                PlayBuffer(SOUND_CLICK01);
                return false;
            }
        }

        if (m_iPage > 0 && CheckMouseIn(m_Pos.x + PagerLeftX(), m_Pos.y + PAGER_Y, PAGER_ARROW_WIDTH, PAGER_HEIGHT))
        {
            SwitchPage(m_iPage - 1);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
        if (m_iPage < JewelBank::PAGE_COUNT - 1 && CheckMouseIn(m_Pos.x + PagerRightX(), m_Pos.y + PAGER_Y, PAGER_ARROW_WIDTH, PAGER_HEIGHT))
        {
            SwitchPage(m_iPage + 1);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        int row = 0;
        bool onAmount = false;
        if (RowAt(MouseX, MouseY, row, onAmount))
        {
            if (onAmount)
            {
                BeginEdit(row);
            }
            else
            {
                const JewelBank::Available available = CurrentAvailable();
                CurrentAmounts()[row] = JewelBank::ToggleAmount(CurrentAmounts()[row], available[row]);
                RefreshButtons();
            }
            PlayBuffer(SOUND_CLICK01);
            return false;
        }

        // The auto-deposit tick (Deposit tab only): the box and its label are one click target.
        if (m_iTab == TAB_DEPOSIT && !m_bOperationPending && CheckMouseIn(m_Pos.x + LIST_X, m_Pos.y + AUTO_Y, LIST_WIDTH, 16))
        {
            SendAutoDeposit(!m_bAutoDeposit);
            PlayBuffer(SOUND_CLICK01);
            return false;
        }
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

    if (m_iEditRow >= 0)
    {
        if (SEASON3B::IsPress(VK_RETURN) == true)
        {
            CommitEdit();
            return false;
        }
        if (SEASON3B::IsPress(VK_ESCAPE) == true)
        {
            CancelEdit();
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
    if (!IsVisible())
    {
        return true;
    }

    const auto now = Clock::now();
    if (m_bOperationPending && now >= m_OperationDeadline)
    {
        // No answer in time: let the player try again rather than keeping the buttons locked.
        m_bOperationPending = false;
        ShowMessage(L"No answer from the server. Try again.", false);
        RefreshButtons();
    }

    if (!m_bOperationPending && now >= m_NextPoll)
    {
        // What the player carries changes while the window is open (pickups, trades), so the counts are asked for again.
        SendOpenRequest();
        m_NextPoll = now + POLL_INTERVAL;
    }
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

    RenderTabs();
    RenderHeader();
    RenderRows();
    RenderPager();
    RenderActions();

    EnableAlphaTest();
    return true;
}

void SEASON3B::CNewUIJewelBankWindow::Render3D()
{
    if (!IsVisible())
    {
        return;
    }

    for (int r = 0; r < JewelBank::ROW_COUNT; ++r)
    {
        const KindIcon icon = IconOf(JewelBank::KindOnPage(m_iPage, r));
        RenderItem3DFittedToSlot(static_cast<float>(m_Pos.x + LIST_X + ICON_X), static_cast<float>(m_Pos.y + RowY(r) + (ROW_HEIGHT - ICON_SIZE) / 2),
            static_cast<float>(ICON_SIZE), static_cast<float>(ICON_SIZE), icon.type, icon.level, 0, 0, icon.fill, false);
    }
}

void SEASON3B::CNewUIJewelBankWindow::RenderFrame()
{
    const float x = static_cast<float>(m_Pos.x);
    const float y = static_cast<float>(m_Pos.y);
    const float centerWidth = static_cast<float>(WINDOW_WIDTH - 2 * FRAME_CAP_WIDTH);

    // The same construction as the Auction House frame, with its custom background image stretched over the window: the two ornate
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

void SEASON3B::CNewUIJewelBankWindow::RenderTabs()
{
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const wchar_t* const labels[TAB_COUNT] = { L"Deposit", L"Withdraw" };
    for (int tab = 0; tab < TAB_COUNT; ++tab)
    {
        const int x = m_Pos.x + TabX(tab);
        const int y = m_Pos.y + TAB_Y;
        const bool active = tab == m_iTab;
        const bool hovered = !active && CheckMouseIn(x, y, TAB_WIDTH, TAB_HEIGHT);
        UI::NineSlice::Render(UI::AuctionChrome::AuctionButtonSkin().texture, static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(TAB_WIDTH), static_cast<float>(TAB_HEIGHT),
            active ? TAB_TINT_ACTIVE : (hovered ? TAB_TINT_HOVER : TAB_TINT_IDLE));
        if (active)
        {
            g_pRenderText->SetTextColor(255, 226, 130, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(235, 235, 235, 255);
        }
        g_pRenderText->RenderText(static_cast<float>(x), static_cast<float>(y + 4), labels[tab], static_cast<float>(TAB_WIDTH), 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CNewUIJewelBankWindow::RenderPager()
{
    using Storage = SEASON3B::CNewUIStorageInventory;
    const int y = m_Pos.y + PAGER_Y;
    for (int arrow = 0; arrow < 2; ++arrow)
    {
        const bool left = arrow == 0;
        const bool enabled = left ? m_iPage > 0 : m_iPage < JewelBank::PAGE_COUNT - 1;
        const int x = m_Pos.x + (left ? PagerLeftX() : PagerRightX());
        const bool hovered = enabled && CheckMouseIn(x, y, PAGER_ARROW_WIDTH, PAGER_HEIGHT);
        const DWORD color = !enabled ? RGBA(80, 80, 80, 255) : (hovered ? RGBA(255, 255, 255, 255) : RGBA(205, 205, 205, 255));
        RenderImage(left ? Storage::IMAGE_STORAGE_VAULT_ARROW_L : Storage::IMAGE_STORAGE_VAULT_ARROW_R,
            static_cast<float>(x), static_cast<float>(y), static_cast<float>(PAGER_ARROW_WIDTH), static_cast<float>(PAGER_HEIGHT), 0.0f, 0.0f, color);
    }
    EnableAlphaTest();

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 226, 130, 255);
    const std::wstring label = std::wstring(JewelBank::PageName(m_iPage)) + L"  " + std::to_wstring(m_iPage + 1) + L"/" + std::to_wstring(JewelBank::PAGE_COUNT);
    g_pRenderText->RenderText(static_cast<float>(m_Pos.x + PagerLeftX() + PAGER_ARROW_WIDTH + PAGER_GAP), static_cast<float>(y + 6), label.c_str(),
        static_cast<float>(PAGER_LABEL_WIDTH), 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIJewelBankWindow::RenderHeader()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    const int left = m_Pos.x + LIST_X;
    const int y = m_Pos.y + HEADER_Y;
    const bool deposit = m_iTab == TAB_DEPOSIT;

    SetTextColorArgb(0xFFB4B4B4u);
    g_pRenderText->RenderText(static_cast<float>(left + NAME_X), static_cast<float>(y), L"Item", 0, 0, RT3_SORT_LEFT);

    // The column the tab takes from is the one in gold.
    SetTextColorArgb(deposit ? 0xFFB4B4B4u : 0xFFFFD25Au);
    g_pRenderText->RenderText(static_cast<float>(left + BANK_RIGHT - BANK_WIDTH), static_cast<float>(y), L"In bank", static_cast<float>(BANK_WIDTH), 0, RT3_SORT_RIGHT);
    SetTextColorArgb(deposit ? 0xFFFFD25Au : 0xFFB4B4B4u);
    g_pRenderText->RenderText(static_cast<float>(left + INVENTORY_RIGHT - INVENTORY_WIDTH), static_cast<float>(y), L"In inventory", static_cast<float>(INVENTORY_WIDTH), 0, RT3_SORT_RIGHT);
    SetTextColorArgb(0xFFB4B4B4u);
    g_pRenderText->RenderText(static_cast<float>(left + AMOUNT_X), static_cast<float>(y), deposit ? L"Deposit" : L"Withdraw", static_cast<float>(AMOUNT_WIDTH), 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIJewelBankWindow::RenderRows()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    int hovered = -1;
    bool hoveredOnAmount = false;
    RowAt(MouseX, MouseY, hovered, hoveredOnAmount);
    const bool deposit = m_iTab == TAB_DEPOSIT;
    const JewelBank::Available available = CurrentAvailable();
    const JewelBank::Amounts& amounts = CurrentAmounts();
    const UI::NineSlice::StateTints& fieldTints = UI::AuctionChrome::AuctionFieldSkin().tints;

    const int rowHeight = ROW_HEIGHT;
    const int textY = AMOUNT_TEXT_Y;
    for (int r = 0; r < JewelBank::ROW_COUNT; ++r)
    {
        const int i = JewelBank::KindOnPage(m_iPage, r);
        const JewelBank::KindInfo& kind = JewelBank::KindAt(i);
        const int rowX = m_Pos.x + LIST_X;
        const int rowY = m_Pos.y + RowY(r);
        const bool selected = amounts[i] > 0;
        UI::NineSlice::Render(UI::AuctionChrome::AuctionRowSkin().texture, static_cast<float>(rowX), static_cast<float>(rowY),
            static_cast<float>(LIST_WIDTH), static_cast<float>(rowHeight),
            selected ? ROW_TINT_SELECTED : (i == hovered && !hoveredOnAmount ? ROW_TINT_HOVER : ROW_TINT_IDLE));

        // The item model is drawn in the 3D pass (Render3D). A selected row is shown by its gold ring and gold text.

        const bool hasAny = available[i] > 0;
        g_pRenderText->SetTextColor(selected ? 255 : 230, selected ? 226 : 230, selected ? 130 : 230, 255);
        g_pRenderText->RenderText(static_cast<float>(rowX + NAME_X), static_cast<float>(rowY + textY), kind.Name, 0, 0, RT3_SORT_LEFT);

        const uint64_t bank = m_Counts[i];
        const uint64_t bag = m_Inventory[i];
        const auto renderCount = [&](uint64_t value, int right, int width, bool source)
        {
            if (value == 0)
            {
                g_pRenderText->SetTextColor(115, 115, 115, 255);
            }
            else if (source)
            {
                g_pRenderText->SetTextColor(255, 220, 130, 255);
            }
            else
            {
                g_pRenderText->SetTextColor(200, 200, 200, 255);
            }
            g_pRenderText->RenderText(static_cast<float>(rowX + right - width), static_cast<float>(rowY + textY),
                JewelBank::FormatCount(value).c_str(), static_cast<float>(width), 0, RT3_SORT_RIGHT);
        };
        renderCount(bank, BANK_RIGHT, BANK_WIDTH, !deposit);
        renderCount(bag, INVENTORY_RIGHT, INVENTORY_WIDTH, deposit);

        // The amount cell: a recessed field, gold ring while it is being edited.
        const int cellX = rowX + AMOUNT_X;
        UI::NineSlice::Render(UI::AuctionChrome::AuctionFieldSkin().texture, static_cast<float>(cellX), static_cast<float>(rowY),
            static_cast<float>(AMOUNT_WIDTH), static_cast<float>(rowHeight),
            m_iEditRow == i ? fieldTints.pressed : (i == hovered && hoveredOnAmount && hasAny ? fieldTints.hover : fieldTints.normal));
        if (m_iEditRow == i)
        {
            // Typed digits sit where the shown amount sits: centred in the cell, on the same line as the other texts.
            wchar_t typed[16]{};
            m_AmountInput.GetText(typed, static_cast<int>(std::size(typed)));
            const int textWidth = static_cast<int>(g_pRenderText->MeasureText(typed, static_cast<int>(wcslen(typed))).cx);
            m_AmountInput.SetPosition(cellX + std::max(AMOUNT_INSET, (AMOUNT_WIDTH - textWidth) / 2), rowY + textY);
            m_AmountInput.Render();
        }
        else
        {
            if (amounts[i] > 0)
            {
                g_pRenderText->SetTextColor(255, 220, 130, 255);
            }
            else
            {
                g_pRenderText->SetTextColor(115, 115, 115, 255);
            }
            g_pRenderText->RenderText(static_cast<float>(cellX), static_cast<float>(rowY + textY),
                amounts[i] > 0 ? std::to_wstring(amounts[i]).c_str() : L"-", static_cast<float>(AMOUNT_WIDTH), 0, RT3_SORT_CENTER);
        }
    }
}

void SEASON3B::CNewUIJewelBankWindow::RenderActions()
{
    m_BtnSelectAll.Render();
    m_BtnClear.Render();
    m_BtnAction.Render();

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0, 0, 0, 0);

    // The auto-deposit tick belongs to the Deposit tab: it decides where picked-up jewels and fruits go.
    if (m_iTab == TAB_DEPOSIT)
    {
        // A round tick: a light ring, a dark centre, and a gold dot when on.
        const float centerX = static_cast<float>(m_Pos.x + LIST_X + CHECK_X) + CHECK_SIZE * 0.5f;
        const float centerY = static_cast<float>(m_Pos.y + AUTO_Y + 2) + CHECK_SIZE * 0.5f;
        FillCircle(centerX, centerY, CHECK_SIZE * 0.5f, 0xFFB8B8B8u);
        FillCircle(centerX, centerY, CHECK_SIZE * 0.5f - 1.f, 0xFF15181Eu);
        if (m_bAutoDeposit)
        {
            FillCircle(centerX, centerY, CHECK_SIZE * 0.5f - 2.6f, 0xFFFFD25Au);
        }
        EndRenderColor();
        EnableAlphaTest();
        g_pRenderText->SetTextColor(220, 220, 200, 255);
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x + LIST_X + CHECK_X + CHECK_SIZE + 8), static_cast<float>(m_Pos.y + AUTO_Y + 3),
            L"Send picked-up items to the bank", 0, 0, RT3_SORT_LEFT);
    }

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
    else
    {
        g_pRenderText->SetTextColor(150, 150, 150, 255);
        g_pRenderText->RenderText(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y + MESSAGE_Y),
            L"Click a row to select it, or type an amount.", static_cast<float>(WINDOW_WIDTH), 0, RT3_SORT_CENTER);
    }
}
