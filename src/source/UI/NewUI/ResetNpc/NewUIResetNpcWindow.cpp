#include "stdafx.h"
#include "UI/NewUI/ResetNpc/NewUIResetNpcWindow.h"

#include <cmath>

#include "Audio/DSPlaySound.h"
#include "Network/Server/SocketSystem.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/AuctionHouse/AuctionChrome.h"
#include "UI/NewUI/HUD/BuffTooltip.h"
#include "UI/NewUI/HUD/NewUIBuffWindow.h"
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
    constexpr int IconCentreX = 28;
    constexpr int LabelInset = 50;
    constexpr int MarkInset = 26;
    constexpr int ValueGapToMark = 12;
    constexpr int MessageY = 226;
    constexpr int HintY = 242;
    constexpr int ButtonY = 256;
    constexpr int ButtonWidth = 170;
    constexpr int ButtonHeight = 24;
    constexpr int CloseWidth = 36;
    constexpr int CloseHeight = 29;
    constexpr int CloseMargin = 8;
    constexpr int WindowTop = 50;
    constexpr int HelpRadius = 9;
    constexpr int HelpGapToHeading = 22;
    constexpr int TooltipGap = 6;
    constexpr float IconRadius = 10.f;
    constexpr std::chrono::seconds ConfirmTime{5};

    constexpr unsigned int MetColor = 0xFF7EE28Cu;
    constexpr unsigned int UnmetColor = 0xFFFF7E74u;
    constexpr unsigned int RimDark = 0xFF1A1208u;
    constexpr unsigned int RimGold = 0xFFB38B35u;
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

    unsigned int Blend(unsigned int from, unsigned int to, float share)
    {
        const auto mix = [share](unsigned int a, unsigned int b)
        {
            return static_cast<unsigned int>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * share);
        };
        return 0xFF000000u | (mix((from >> 16) & 0xFF, (to >> 16) & 0xFF) << 16) | (mix((from >> 8) & 0xFF, (to >> 8) & 0xFF) << 8)
            | mix(from & 0xFF, to & 0xFF);
    }

    // A disc drawn as one-pixel strips, shaded from the top colour to the bottom colour.
    void DrawDisc(float centreX, float centreY, float radius, unsigned int top, unsigned int bottom)
    {
        const int rows = static_cast<int>(std::ceil(radius * 2.f));
        for (int row = 0; row < rows; ++row)
        {
            const float offset = (static_cast<float>(row) + 0.5f) - radius;
            const float half = std::sqrt(std::max(0.f, radius * radius - offset * offset));
            const float shade = static_cast<float>(row) / static_cast<float>(std::max(1, rows - 1));
            RenderColorQuadARGB(centreX - half, centreY - radius + static_cast<float>(row), half * 2.f, 1.f, Blend(top, bottom, shade));
        }
    }

    void DrawLine(float x1, float y1, float x2, float y2, float thickness, unsigned int color)
    {
        RenderColorLineARGB(x1, y1, x2, y2, thickness, color);
    }

    // The round frame of an icon: a dark edge, a gold rim and the shaded inside.
    void DrawMedallion(float centreX, float centreY, unsigned int insideTop, unsigned int insideBottom)
    {
        DrawDisc(centreX, centreY, IconRadius, RimDark, RimDark);
        DrawDisc(centreX, centreY, IconRadius - 0.8f, 0xFFE6C477u, RimGold);
        DrawDisc(centreX, centreY, IconRadius - 2.f, insideTop, insideBottom);
    }

    // Two chevrons pointing up: the level.
    void DrawLevelIcon(float x, float y)
    {
        DrawMedallion(x, y, 0xFF3A5A9Au, 0xFF18284Cu);
        for (int step = 0; step < 2; ++step)
        {
            const float lift = static_cast<float>(step) * 4.5f;
            DrawLine(x - 4.5f, y + 3.f - lift, x, y - 1.5f - lift, 2.f, 0xFFFFE9A8u);
            DrawLine(x, y - 1.5f - lift, x + 4.5f, y + 3.f - lift, 2.f, 0xFFFFE9A8u);
        }
    }

    // A gold coin with a Z: the Zen.
    void DrawZenIcon(float x, float y)
    {
        DrawMedallion(x, y, 0xFFF6D77Au, 0xFFC08A22u);
        const unsigned int ink = 0xFF5A3A08u;
        DrawLine(x - 3.2f, y - 3.2f, x + 3.2f, y - 3.2f, 1.6f, ink);
        DrawLine(x + 3.2f, y - 3.2f, x - 3.2f, y + 3.2f, 1.6f, ink);
        DrawLine(x - 3.2f, y + 3.2f, x + 3.2f, y + 3.2f, 1.6f, ink);
    }

    // A fruit with a stem and a leaf.
    void DrawFruitIcon(float x, float y)
    {
        DrawMedallion(x, y, 0xFF3C7A3Cu, 0xFF163A1Cu);
        DrawDisc(x, y + 1.5f, 4.6f, 0xFFFF8A5Cu, 0xFFC02A1Cu);
        DrawLine(x, y - 3.f, x + 0.5f, y - 6.f, 1.4f, 0xFF7A4A1Au);
        DrawDisc(x + 3.2f, y - 4.4f, 2.2f, 0xFF9CE06Au, 0xFF3F9A2Au);
    }

    // A shield: the equipment.
    void DrawEquipmentIcon(float x, float y)
    {
        DrawMedallion(x, y, 0xFF4A4F5Cu, 0xFF1C1F28u);
        for (int row = 0; row < 11; ++row)
        {
            const float yy = y - 5.5f + static_cast<float>(row);
            const float half = row < 6 ? 4.6f : std::max(0.6f, 4.6f - static_cast<float>(row - 5) * 0.85f);
            RenderColorQuadARGB(x - half, yy, half * 2.f, 1.f, Blend(0xFFE4E8F2u, 0xFF8C93A6u, static_cast<float>(row) / 10.f));
        }
    }

    void DrawCheck(float x, float y, unsigned int color)
    {
        DrawLine(x - 5.f, y, x - 1.5f, y + 3.5f, 2.f, color);
        DrawLine(x - 1.5f, y + 3.5f, x + 5.f, y - 4.f, 2.f, color);
    }

    void DrawCross(float x, float y, unsigned int color)
    {
        DrawLine(x - 4.f, y - 4.f, x + 4.f, y + 4.f, 2.f, color);
        DrawLine(x - 4.f, y + 4.f, x + 4.f, y - 4.f, 2.f, color);
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

SEASON3B::CNewUIResetNpcWindow::Rect SEASON3B::CNewUIResetNpcWindow::HelpRect() const
{
    const int centreX = m_posX + (WINDOW_WIDTH - HeadingWidth) / 2 - HelpGapToHeading;
    const int centreY = m_posY + HeadingY + HeadingHeight / 2;
    return { centreX - HelpRadius, centreY - HelpRadius, HelpRadius * 2, HelpRadius * 2 };
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

    const float iconX = x + IconCentreX - RowInset + 8.f;
    const float iconY = y + RowHeight / 2.f;
    switch (icon)
    {
    case RowIcon::Level: DrawLevelIcon(iconX, iconY); break;
    case RowIcon::Zen: DrawZenIcon(iconX, iconY); break;
    case RowIcon::Fruit: DrawFruitIcon(iconX, iconY); break;
    case RowIcon::Equipment: DrawEquipmentIcon(iconX, iconY); break;
    }

    const unsigned int color = met ? MetColor : UnmetColor;
    const float markX = x + width - MarkInset + 4.f;
    if (met)
    {
        DrawCheck(markX, iconY, color);
    }
    else
    {
        DrawCross(markX, iconY, color);
    }
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
    const Rect help = HelpRect();
    const bool hovered = SEASON3B::CheckMouseIn(help.x, help.y, help.width, help.height);
    const float centreX = help.x + help.width / 2.f;
    const float centreY = help.y + help.height / 2.f;
    DrawDisc(centreX, centreY, static_cast<float>(HelpRadius), RimDark, RimDark);
    DrawDisc(centreX, centreY, HelpRadius - 0.8f, 0xFFE6C477u, RimGold);
    DrawDisc(centreX, centreY, HelpRadius - 2.f, hovered ? 0xFF5A4A28u : 0xFF2E2614u, hovered ? 0xFF3A2E14u : 0xFF16120Au);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0, 0, 0, 0);
    g_pRenderText->SetTextColor(255, 226, 150, 255);
    const SIZE size = g_pRenderText->MeasureText(L"?", 1);
    g_pRenderText->RenderText(static_cast<float>(help.x), centreY - size.cy / 2.f, L"?", static_cast<float>(help.width), 0, RT3_SORT_CENTER);
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

    const float headingX = static_cast<float>(m_posX + (WINDOW_WIDTH - HeadingWidth) / 2);
    UI::NineSlice::Render(UI::AuctionChrome::AuctionButtonSkin().texture, headingX, static_cast<float>(m_posY + HeadingY),
        static_cast<float>(HeadingWidth), static_cast<float>(HeadingHeight), HeadingTint);
    g_pRenderText->SetTextColor(255, 236, 170, 255);
    const std::wstring heading = ResetNpc::HeadingText(m_state);
    const SIZE headingSize = g_pRenderText->MeasureText(heading.c_str(), static_cast<int>(heading.size()));
    g_pRenderText->RenderText(headingX, static_cast<float>(m_posY + HeadingY) + (HeadingHeight - headingSize.cy) / 2.f,
        heading.c_str(), static_cast<float>(HeadingWidth), 0, RT3_SORT_CENTER);
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
        RenderRequirementRow(row++, RowIcon::Fruit, L"Fruits", L"not needed", true);
    }
    if (m_state.GearRuleActive())
    {
        const std::wstring worn = m_state.WornItems == 0 ? std::wstring(L"nothing worn") : std::to_wstring(m_state.WornItems) + L" worn";
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
