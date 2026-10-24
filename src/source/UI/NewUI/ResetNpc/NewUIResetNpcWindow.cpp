#include "stdafx.h"
#include "UI/NewUI/ResetNpc/NewUIResetNpcWindow.h"

#include "Audio/DSPlaySound.h"
#include "Network/Server/SocketSystem.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/NewUISystem.h"

namespace
{
    constexpr int TitleY = 13;
    constexpr int HeadingY = 40;
    constexpr int HeadingWidth = 170;
    constexpr int HeadingHeight = 22;
    constexpr int RowsTop = 70;
    constexpr int RowHeight = 34;
    constexpr int RowGap = 4;
    constexpr int RowInset = 22;
    constexpr int RowTextInset = 36;
    constexpr int MarkerSize = 10;
    constexpr int MarkerInset = 14;
    constexpr int MessageY = 190;
    constexpr int HintY = 206;
    constexpr int ButtonY = 220;
    constexpr int ButtonWidth = 170;
    constexpr int ButtonHeight = 24;
    constexpr int CloseWidth = 36;
    constexpr int CloseHeight = 29;
    constexpr int CloseMargin = 8;
    constexpr int WindowTop = 50;
    constexpr std::chrono::seconds ConfirmTime{5};

    constexpr unsigned int MetColor = 0xFF7EE28Cu;
    constexpr unsigned int UnmetColor = 0xFFFF7E74u;
    const DWORD HeadingTint = RGBA(255, 214, 118, 255);
    const DWORD RowTint = RGBA(104, 110, 122, 255);
    const DWORD ButtonTintReady = RGBA(255, 214, 118, 255);
    const DWORD ButtonTintConfirm = RGBA(255, 150, 110, 255);
    const DWORD ButtonTintHover = RGBA(255, 236, 170, 255);
    const DWORD ButtonTintLocked = RGBA(88, 90, 96, 255);

    void SetTextColorArgb(unsigned int argb)
    {
        g_pRenderText->SetTextColor((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, argb >> 24);
    }
}

SEASON3B::CNewUIResetNpcWindow::CNewUIResetNpcWindow() = default;

SEASON3B::CNewUIResetNpcWindow::~CNewUIResetNpcWindow()
{
    Release();
}

bool SEASON3B::CNewUIResetNpcWindow::Create(CNewUIManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_RESET_NPC, this);
    m_posX = (REFERENCE_WIDTH - WINDOW_WIDTH) / 2;
    m_posY = WindowTop;
    return true;
}

void SEASON3B::CNewUIResetNpcWindow::Release()
{
    if (m_pNewUIMng != nullptr)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIResetNpcWindow::OpeningProcess()
{
    m_confirmPending = false;
    m_waitingForAnswer = false;
    m_message = L"";
}

void SEASON3B::CNewUIResetNpcWindow::ClosingProcess()
{
    m_hasState = false;
    m_confirmPending = false;
    SocketClient->ToGameServer()->SendCloseNpcRequest();
}

void SEASON3B::CNewUIResetNpcWindow::SetState(const ResetNpc::State& state)
{
    if (state.Result == ResetNpc::Outcome::Done)
    {
        // The server logs the character out after the reset; nothing more to show.
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_RESET_NPC);
        return;
    }

    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_RESET_NPC))
    {
        if (state.Result != ResetNpc::Outcome::Info)
        {
            return;
        }
        g_pNewUISystem->Show(SEASON3B::INTERFACE_RESET_NPC);
    }

    // A refusal carries no numbers: keep what the window shows and only say why.
    if (state.Result != ResetNpc::Outcome::Refused || !m_hasState)
    {
        m_state = state;
        m_hasState = true;
    }
    m_message = ResetNpc::OutcomeText(state.Result);
    m_waitingForAnswer = false;
    m_confirmPending = false;
}

SEASON3B::CNewUIResetNpcWindow::Rect SEASON3B::CNewUIResetNpcWindow::ButtonRect() const
{
    return { m_posX + (WINDOW_WIDTH - ButtonWidth) / 2, m_posY + ButtonY, ButtonWidth, ButtonHeight };
}

SEASON3B::CNewUIResetNpcWindow::Rect SEASON3B::CNewUIResetNpcWindow::CloseRect() const
{
    return { m_posX + WINDOW_WIDTH - CloseWidth - CloseMargin, m_posY + CloseMargin, CloseWidth, CloseHeight };
}

void SEASON3B::CNewUIResetNpcWindow::PressResetButton()
{
    if (!m_hasState || !m_state.CanReset() || m_waitingForAnswer)
    {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!m_confirmPending || now > m_confirmDeadline)
    {
        m_confirmPending = true;
        m_confirmDeadline = now + ConfirmTime;
        m_message = L"";
        return;
    }

    m_confirmPending = false;
    m_waitingForAnswer = true;
    SocketClient->ToGameServer()->SendResetNpcResetRequest();
}

bool SEASON3B::CNewUIResetNpcWindow::Update()
{
    if (m_confirmPending && std::chrono::steady_clock::now() > m_confirmDeadline)
    {
        m_confirmPending = false;
    }
    return true;
}

bool SEASON3B::CNewUIResetNpcWindow::UpdateMouseEvent()
{
    // The close button is part of the frame art, as in the Auction House; only its area needs handling.
    const Rect close = CloseRect();
    if (SEASON3B::CheckMouseIn(close.x, close.y, close.width, close.height) && SEASON3B::IsRelease(VK_LBUTTON))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_RESET_NPC);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    const Rect button = ButtonRect();
    if (SEASON3B::CheckMouseIn(button.x, button.y, button.width, button.height) && SEASON3B::IsRelease(VK_LBUTTON))
    {
        PlayBuffer(SOUND_CLICK01);
        PressResetButton();
        return true;
    }

    return !SEASON3B::CheckMouseIn(m_posX, m_posY, WINDOW_WIDTH, WINDOW_HEIGHT);
}

bool SEASON3B::CNewUIResetNpcWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_RESET_NPC) || !SEASON3B::IsPress(VK_ESCAPE))
    {
        return true;
    }

    g_pNewUISystem->Hide(SEASON3B::INTERFACE_RESET_NPC);
    PlayBuffer(SOUND_CLICK01);
    return false;
}

float SEASON3B::CNewUIResetNpcWindow::GetLayerDepth()
{
    return 10.2f;
}

void SEASON3B::CNewUIResetNpcWindow::RenderRequirementRow(int row, const wchar_t* label, const std::wstring& value, bool met) const
{
    const float x = static_cast<float>(m_posX + RowInset);
    const float y = static_cast<float>(m_posY + RowsTop + row * (RowHeight + RowGap));
    const float width = static_cast<float>(WINDOW_WIDTH - 2 * RowInset);
    UI::NineSlice::Render(UI::AuctionChrome::AuctionRowSkin().texture, x, y, width, static_cast<float>(RowHeight), RowTint);

    const unsigned int color = met ? MetColor : UnmetColor;
    RenderColorQuadARGB(x + MarkerInset, y + (RowHeight - MarkerSize) / 2.f, static_cast<float>(MarkerSize), static_cast<float>(MarkerSize), color);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(235, 235, 235, 255);
    const SIZE labelSize = g_pRenderText->MeasureText(label, static_cast<int>(wcslen(label)));
    const float textY = y + (RowHeight - labelSize.cy) / 2.f;
    g_pRenderText->RenderText(x + RowTextInset, textY, label, 0, 0, RT3_SORT_LEFT);

    g_pRenderText->SetFont(g_hFont);
    SetTextColorArgb(color);
    const SIZE valueSize = g_pRenderText->MeasureText(value.c_str(), static_cast<int>(value.size()));
    g_pRenderText->RenderText(x + width - 12 - valueSize.cx, y + (RowHeight - valueSize.cy) / 2.f, value.c_str(), 0, 0, RT3_SORT_LEFT);
}

bool SEASON3B::CNewUIResetNpcWindow::Render()
{
    EnableAlphaTest();
    UI::AuctionChrome::RenderWindowFrame(static_cast<float>(m_posX), static_cast<float>(m_posY),
        static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(220, 220, 220, 255);
    g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + TitleY),
        L"Raymond the Reset Master", static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);

    if (!m_hasState)
    {
        DisableAlphaBlend();
        return true;
    }

    const float headingX = static_cast<float>(m_posX + (WINDOW_WIDTH - HeadingWidth) / 2);
    UI::NineSlice::Render(UI::AuctionChrome::AuctionButtonSkin().texture, headingX, static_cast<float>(m_posY + HeadingY),
        static_cast<float>(HeadingWidth), static_cast<float>(HeadingHeight), HeadingTint);
    g_pRenderText->SetTextColor(255, 236, 170, 255);
    const std::wstring heading = ResetNpc::HeadingText(m_state);
    const SIZE headingSize = g_pRenderText->MeasureText(heading.c_str(), static_cast<int>(heading.size()));
    g_pRenderText->RenderText(headingX, static_cast<float>(m_posY + HeadingY) + (HeadingHeight - headingSize.cy) / 2.f,
        heading.c_str(), static_cast<float>(HeadingWidth), 0, RT3_SORT_CENTER);

    if (m_state.LimitReached())
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(235, 235, 235, 255);
        g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + RowsTop + RowHeight),
            L"You have done every reset there is. Well done!", static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);
        DisableAlphaBlend();
        return true;
    }

    RenderRequirementRow(0, L"Level",
        std::to_wstring(m_state.CurrentLevel) + L" / " + std::to_wstring(m_state.RequiredLevel), m_state.LevelEnough());
    RenderRequirementRow(1, L"Zen",
        ResetNpc::FormatNumber(m_state.CurrentZen) + L" / " + ResetNpc::FormatNumber(m_state.RequiredZen), m_state.ZenEnough());
    if (m_state.RequiredFruits > 0)
    {
        RenderRequirementRow(2, L"Fruits (Jewel Bank)",
            ResetNpc::FormatNumber(m_state.BankFruits) + L" / " + ResetNpc::FormatNumber(m_state.RequiredFruits), m_state.FruitsEnough());
    }
    else
    {
        RenderRequirementRow(2, L"Fruits", L"not needed", true);
    }

    const Rect button = ButtonRect();
    const bool canPress = m_state.CanReset() && !m_waitingForAnswer;
    const bool hovered = canPress && SEASON3B::CheckMouseIn(button.x, button.y, button.width, button.height);
    DWORD tint = ButtonTintLocked;
    if (canPress)
    {
        tint = hovered ? ButtonTintHover : (m_confirmPending ? ButtonTintConfirm : ButtonTintReady);
    }
    UI::NineSlice::Render(UI::AuctionChrome::AuctionButtonSkin().texture, static_cast<float>(button.x), static_cast<float>(button.y),
        static_cast<float>(button.width), static_cast<float>(button.height), tint);

    g_pRenderText->SetFont(g_hFontBold);
    const wchar_t* buttonText = m_waitingForAnswer ? L"Please wait..." : (m_confirmPending ? L"Click again to confirm" : L"Reset");
    if (canPress)
    {
        g_pRenderText->SetTextColor(255, 255, 255, 255);
    }
    else
    {
        g_pRenderText->SetTextColor(150, 150, 150, 255);
    }
    const SIZE buttonSize = g_pRenderText->MeasureText(buttonText, static_cast<int>(wcslen(buttonText)));
    g_pRenderText->RenderText(static_cast<float>(button.x), static_cast<float>(button.y) + (button.height - buttonSize.cy) / 2.f,
        buttonText, static_cast<float>(button.width), 0, RT3_SORT_CENTER);

    g_pRenderText->SetFont(g_hFont);
    if (m_message[0] != L'\0')
    {
        SetTextColorArgb(UnmetColor);
        g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + MessageY),
            m_message, static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);
    }
    else if (m_confirmPending)
    {
        g_pRenderText->SetTextColor(255, 200, 120, 255);
        g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + MessageY),
            L"The reset takes you back to the character selection.", static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);
    }

    if (m_state.RequiredFruits > 0)
    {
        g_pRenderText->SetTextColor(170, 176, 190, 255);
        g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + HintY),
            L"Fruits of any type are taken from your Jewel Bank (Ctrl+J).", static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);
    }

    DisableAlphaBlend();
    return true;
}
