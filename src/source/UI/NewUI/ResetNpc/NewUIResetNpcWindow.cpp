#include "stdafx.h"
#include "UI/NewUI/ResetNpc/NewUIResetNpcWindow.h"

#include "Audio/DSPlaySound.h"
#include "Network/Server/SocketSystem.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/HUD/BuffTooltip.h"
#include "UI/NewUI/HUD/NewUIBuffWindow.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUISystem.h"

namespace
{
    constexpr int TitleY = 13;
    constexpr int HeadingY = 38;
    constexpr int HeadingHeight = 22;
    constexpr int RowsTop = 64;
    constexpr int RowHeight = 32;
    constexpr int RowGap = 4;
    constexpr int RowInset = 20;
    constexpr int LabelInset = 50;
    constexpr int MarkInset = 26;
    constexpr int ValueGapToMark = 12;
    constexpr int MessageY = 204;
    constexpr int HintY = 220;
    constexpr int ButtonY = 240;
    constexpr int ButtonWidth = 170;
    constexpr int ButtonHeight = 24;
    constexpr int CloseWidth = 36;
    constexpr int CloseHeight = 29;
    constexpr int CloseMargin = 8;
    constexpr int WindowTop = 50;
    constexpr int HeadingHitPadding = 6;
    constexpr float UnderlineGap = 1.f;
    constexpr int TooltipGap = 6;
    constexpr float IconSize = 20.f;
    constexpr float FruitSize = 16.f;
    constexpr float MarkSize = 11.f;
    constexpr std::chrono::seconds ConfirmTime{5};

    constexpr unsigned int MetColor = 0xFF7EE28Cu;
    constexpr unsigned int UnmetColor = 0xFFFF7E74u;
    const DWORD RowTint = RGBA(104, 110, 122, 255);
    const DWORD ButtonTintReady = RGBA(255, 214, 118, 255);
    const DWORD ButtonTintConfirm = RGBA(255, 150, 110, 255);
    const DWORD ButtonTintHover = RGBA(255, 236, 170, 255);
    const DWORD ButtonTintLocked = RGBA(88, 90, 96, 255);

    void SetTextColorArgb(unsigned int argb)
    {
        g_pRenderText->SetTextColor((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, argb >> 24);
    }

    DWORD ArgbToRgba(unsigned int argb)
    {
        return RGBA((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF, argb >> 24);
    }

    // The glyph sheet holds four cells side by side: level, check, cross, help.
    enum GlyphCell
    {
        GlyphLevel = 0,
        GlyphCheck = 1,
        GlyphCross = 2,
        GlyphHelp = 3,
        GlyphShield = 4,
    };

    void DrawGlyph(GlyphCell cell, float x, float y, float size, DWORD color = RGBA(255, 255, 255, 255))
    {
        constexpr float cells = 8.f;
        SEASON3B::RenderImage(SEASON3B::CNewUIResetNpcWindow::IMAGE_RESET_GLYPHS, x - size / 2.f, y - size / 2.f, size, size,
            static_cast<float>(cell) / cells, 0.f, 1.f / cells, 1.f, color);
    }

    // The coin pile of the inventory's Zen counter (the left 33 x 26 pixels of its texture).
    void DrawZenIcon(float x, float y)
    {
        constexpr float width = IconSize;
        constexpr float height = width * 26.f / 33.f;
        SEASON3B::RenderImage(SEASON3B::CNewUIMyInventory::IMAGE_INVENTORY_MONEY, x - width / 2.f, y - height / 2.f, width, height,
            0.f, 0.f, 33.f / 256.f, 26.f / 32.f);
    }

    // The fruit picture of the Jewel Bank.
    void DrawFruitIcon(float x, float y)
    {
        SEASON3B::RenderImage(SEASON3B::CNewUIResetNpcWindow::IMAGE_RESET_FRUIT, x - FruitSize / 2.f, y - FruitSize / 2.f, FruitSize, FruitSize,
            0.f, 0.f, 1.f, 1.f);
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
    LoadBitmap(L"Interface\\newui_reset_fruit.tga", IMAGE_RESET_FRUIT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_reset_glyphs.tga", IMAGE_RESET_GLYPHS, GL_LINEAR);
    return true;
}

void SEASON3B::CNewUIResetNpcWindow::Release()
{
    if (m_pNewUIMng != nullptr)
    {
        DeleteBitmap(IMAGE_RESET_GLYPHS);
        DeleteBitmap(IMAGE_RESET_FRUIT);
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

SEASON3B::CNewUIResetNpcWindow::Rect SEASON3B::CNewUIResetNpcWindow::HelpRect() const
{
    // The heading itself is what explains the requirements: its text, with a little room around it.
    g_pRenderText->SetFont(g_hFontBold);
    const std::wstring heading = ResetNpc::HeadingText(m_state);
    const int textWidth = g_pRenderText->MeasureText(heading.c_str(), static_cast<int>(heading.size())).cx;
    const int width = textWidth + 2 * HeadingHitPadding;
    return { m_posX + (WINDOW_WIDTH - width) / 2, m_posY + HeadingY, width, HeadingHeight };
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

void SEASON3B::CNewUIResetNpcWindow::RenderRequirementRow(int row, RowIcon icon, const wchar_t* label, const std::wstring& value, bool met) const
{
    const float x = static_cast<float>(m_posX + RowInset);
    const float y = static_cast<float>(m_posY + RowsTop + row * (RowHeight + RowGap));
    const float width = static_cast<float>(WINDOW_WIDTH - 2 * RowInset);
    UI::NineSlice::Render(UI::AuctionChrome::AuctionRowSkin().texture, x, y, width, static_cast<float>(RowHeight), RowTint);

    const float iconX = x + 20.f;
    const float iconY = y + RowHeight / 2.f;
    switch (icon)
    {
    case RowIcon::Level: DrawGlyph(GlyphLevel, iconX, iconY, 18.f); break;
    case RowIcon::Zen: DrawZenIcon(iconX, iconY); break;
    case RowIcon::Fruit: DrawFruitIcon(iconX, iconY); break;
    case RowIcon::Equipment: DrawGlyph(GlyphShield, iconX, iconY, 20.f); break;
    }

    const unsigned int color = met ? MetColor : UnmetColor;
    const float markX = x + width - MarkInset + 4.f;
    DrawGlyph(met ? GlyphCheck : GlyphCross, markX, iconY, MarkSize, ArgbToRgba(color));
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(235, 235, 235, 255);
    const SIZE labelSize = g_pRenderText->MeasureText(label, static_cast<int>(wcslen(label)));
    g_pRenderText->RenderText(x + LabelInset - 12.f, y + (RowHeight - labelSize.cy) / 2.f, label, 0, 0, RT3_SORT_LEFT);

    g_pRenderText->SetFont(g_hFont);
    SetTextColorArgb(color);
    const SIZE valueSize = g_pRenderText->MeasureText(value.c_str(), static_cast<int>(value.size()));
    g_pRenderText->RenderText(markX - ValueGapToMark - valueSize.cx, y + (RowHeight - valueSize.cy) / 2.f, value.c_str(), 0, 0, RT3_SORT_LEFT);
}

void SEASON3B::CNewUIResetNpcWindow::RenderHelpButton() const
{
    // A thin gold line under the heading tells that it can be pointed at; it brightens while the tooltip is shown.
    const Rect help = HelpRect();
    const bool hovered = SEASON3B::CheckMouseIn(help.x, help.y, help.width, help.height);
    const float lineY = static_cast<float>(help.y + help.height) - 4.f + UnderlineGap;
    RenderColorQuadARGB(static_cast<float>(help.x + HeadingHitPadding), lineY, static_cast<float>(help.width - 2 * HeadingHitPadding), 1.f,
        hovered ? 0xFFFFE9A8u : 0x99B38B35u);
    EndRenderColor();
}

void SEASON3B::CNewUIResetNpcWindow::RenderHelpTooltip() const
{
    const Rect help = HelpRect();
    if (!SEASON3B::CheckMouseIn(help.x, help.y, help.width, help.height))
    {
        return;
    }

    int count = 0;
    UI::Buffs::Tooltip::Append(L"Reset requirements", TEXT_COLOR_YELLOW, true, count);
    UI::Buffs::Tooltip::AppendDivider(count);
    UI::Buffs::Tooltip::Append(L"Resets 1 to 5: level 400 and 25,000,000 Zen times the number of the reset (25 million for the first, 50 million for the second, up to 125 million for the fifth).", TEXT_COLOR_WHITE, false, count);
    UI::Buffs::Tooltip::Append(L"Reset 6: level 400, 50,000,000 Zen and 100 fruits.", TEXT_COLOR_WHITE, false, count);
    UI::Buffs::Tooltip::Append(L"Resets 7 to 50: level 400, 50,000,000 Zen and 100 more fruits than the reset before (200 at reset 7, 4,500 at reset 50).", TEXT_COLOR_WHITE, false, count);
    UI::Buffs::Tooltip::Append(L"Fruits of any type are taken from your Jewel Bank (Ctrl+J). Deposit them there before you come here.", TEXT_COLOR_GREEN, false, count);
    UI::Buffs::Tooltip::Append(L"You must wear nothing: take off all your equipment first. The items may stay in your inventory.", TEXT_COLOR_RED, false, count);
    UI::Buffs::Tooltip::Append(L"A reset sets your level back and takes you to the character selection.", TEXT_COLOR_GRAY, false, count);
    UI::Buffs::Tooltip::Render(help.x + help.width / 2, help.y + help.height + TooltipGap, count,
        SEASON3B::CNewUIBuffWindow::IMAGE_BUFF_TOOLTIP_ORNAMENTS);
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

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(255, 214, 118, 255);
    const std::wstring heading = ResetNpc::HeadingText(m_state);
    const SIZE headingSize = g_pRenderText->MeasureText(heading.c_str(), static_cast<int>(heading.size()));
    g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + HeadingY) + (HeadingHeight - headingSize.cy) / 2.f,
        heading.c_str(), static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);
    RenderHelpButton();

    if (m_state.LimitReached())
    {
        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetTextColor(235, 235, 235, 255);
        g_pRenderText->RenderText(static_cast<float>(m_posX + 15), static_cast<float>(m_posY + RowsTop + RowHeight),
            L"You have done every reset there is. Well done!", static_cast<float>(WINDOW_WIDTH - 30), 0, RT3_SORT_CENTER);
        RenderHelpTooltip();
        DisableAlphaBlend();
        return true;
    }

    int row = 0;
    RenderRequirementRow(row++, RowIcon::Level, L"Level",
        std::to_wstring(m_state.CurrentLevel) + L" / " + std::to_wstring(m_state.RequiredLevel), m_state.LevelEnough());
    RenderRequirementRow(row++, RowIcon::Zen, L"Zen",
        ResetNpc::FormatNumber(m_state.CurrentZen) + L" / " + ResetNpc::FormatNumber(m_state.RequiredZen), m_state.ZenEnough());
    if (m_state.RequiredFruits > 0)
    {
        RenderRequirementRow(row++, RowIcon::Fruit, L"Fruits (Jewel Bank)",
            ResetNpc::FormatNumber(m_state.BankFruits) + L" / " + ResetNpc::FormatNumber(m_state.RequiredFruits), m_state.FruitsEnough());
    }
    else
    {
        RenderRequirementRow(row++, RowIcon::Fruit, L"Fruits", L"Not required for this reset", true);
    }
    if (m_state.GearRuleActive())
    {
        const std::wstring worn = m_state.GearOk() ? std::wstring(L"Nothing worn") : std::wstring(L"Remove worn items");
        RenderRequirementRow(row++, RowIcon::Equipment, L"Equipment", worn, m_state.GearOk());
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

    RenderHelpTooltip();
    DisableAlphaBlend();
    return true;
}
