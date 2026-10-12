//////////////////////////////////////////////////////////////////////
// NewUIComboBox.cpp: minimal click-to-open dropdown widget.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UI/NewUI/Widgets/NewUIComboBox.h"
#include "UI/NewUI/Widgets/NineSlice.h"
#include "UI/Legacy/UIControls.h"  // for g_pRenderText macro
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"

#include "Render/Textures/ZzzOpenglUtil.h"

extern unsigned int WindowWidth, WindowHeight;
extern int MouseWheel;

using namespace SEASON3B;

namespace
{
    // Visual style for the combo widget. Tweaked to match the option window's
    // existing arrow-button look (see NewUIOptionWindow::RenderContents).
    constexpr float BG_BRIGHTNESS_IDLE     = 0.05f;
    constexpr float BG_BRIGHTNESS_HOVER    = 0.15f;
    constexpr float BG_BRIGHTNESS_OPEN     = 0.10f;  // closed field shade while dropdown is open
    constexpr float SCROLLBAR_TRACK_BRIGHT = 0.02f;
    constexpr float SCROLLBAR_THUMB_BRIGHT = 0.35f;

    constexpr int TEXT_PAD_X     = 6;   // left padding inside rows
    constexpr int TEXT_PAD_Y     = 4;   // top  padding inside rows
    constexpr int ARROW_WIDTH    = 14;  // width reserved for the dropdown-arrow glyph
    constexpr int SCROLLBAR_WIDTH = 6;  // vertical scrollbar width inside the list

    // Vector chevron drawn on a skinned field: a stack of centred one-pixel bars narrowing to a point.
    constexpr int CHEVRON_ROWS        = 3;
    constexpr int CHEVRON_TOP_WIDTH   = 7;
    constexpr int CHEVRON_ROW_NARROW  = 2;    // each row is this much narrower than the one above
    constexpr int CHEVRON_RIGHT_INSET = 11;   // from the field's right edge to the chevron's centre
    constexpr float CHEVRON_BRIGHTNESS = 0.78f;
    // A skinned dropdown is framed by the field art; rows are inset so they sit inside its one-pixel ring.
    constexpr int SKINNED_LIST_ROW_INSET = 2;

    // Draws a solid-color axis-aligned rectangle in UI coordinates.
    // Matches the raw-GL pattern used by NewUIOptionWindow.
    void DrawSolidRect(int x, int y, int w, int h, float brightness)
    {
        mu::GetRenderer().SetTexture2D(false);

        float gx = ConvertPositionX((float)x);
        float gy = ConvertPositionY((float)y);
        float gw = ConvertX((float)w);
        float gh = ConvertY((float)h);
        gy = (float)WindowHeight - gy;

        const std::uint32_t color = mu::PackABGR(brightness, brightness, brightness, 1.f);
        const mu::Vertex2D vertices[4] = {
            {gx, gy, 0.f, 0.f, color},
            {gx, gy - gh, 0.f, 0.f, color},
            {gx + gw, gy - gh, 0.f, 0.f, color},
            {gx + gw, gy, 0.f, 0.f, color},
        };
        mu::GetRenderer().RenderQuad2D(vertices, 0u);
        mu::GetRenderer().SetTexture2D(true);
    }
}

void CNewUIComboBox::Setup(int x, int y, int width, int itemHeight,
                            const wchar_t* const* labels, int itemCount, int initialIdx,
                            int maxVisibleItems)
{
    m_X = x;
    m_Y = y;
    m_Width = width;
    m_ItemHeight = itemHeight;
    m_Labels = labels;
    m_ItemCount = itemCount;
    m_MaxVisibleItems = maxVisibleItems;
    m_ScrollOffset = 0;
    SetSelectedIndex(initialIdx);
    m_bOpen = false;
}

void CNewUIComboBox::SetSelectedIndex(int idx)
{
    if (m_ItemCount <= 0)
    {
        m_SelectedIndex = 0;
        return;
    }
    if (idx < 0) idx = 0;
    if (idx >= m_ItemCount) idx = m_ItemCount - 1;
    m_SelectedIndex = idx;
    ScrollToShowIndex(m_SelectedIndex);
}

int CNewUIComboBox::GetVisibleCount() const
{
    if (m_MaxVisibleItems <= 0 || m_MaxVisibleItems >= m_ItemCount)
        return m_ItemCount;
    return m_MaxVisibleItems;
}

int CNewUIComboBox::GetMaxScrollOffset() const
{
    const int visible = GetVisibleCount();
    return (m_ItemCount > visible) ? (m_ItemCount - visible) : 0;
}

bool CNewUIComboBox::IsScrollable() const
{
    return m_MaxVisibleItems > 0 && m_ItemCount > m_MaxVisibleItems;
}

void CNewUIComboBox::ClampScrollOffset()
{
    const int maxOffset = GetMaxScrollOffset();
    if (m_ScrollOffset < 0)            m_ScrollOffset = 0;
    if (m_ScrollOffset > maxOffset)    m_ScrollOffset = maxOffset;
}

void CNewUIComboBox::ScrollToShowIndex(int idx)
{
    if (!IsScrollable())
    {
        m_ScrollOffset = 0;
        return;
    }
    if (idx < m_ScrollOffset)
        m_ScrollOffset = idx;
    else if (idx >= m_ScrollOffset + m_MaxVisibleItems)
        m_ScrollOffset = idx - m_MaxVisibleItems + 1;
    ClampScrollOffset();
}

int CNewUIComboBox::GetItemIndexAtMouse() const
{
    if (!m_bOpen)
        return -1;

    const int listY = GetListY();
    const int visible = GetVisibleCount();
    // Scrollbar eats the right edge when present
    const int rowWidth = IsScrollable() ? (m_Width - SCROLLBAR_WIDTH) : m_Width;

    for (int row = 0; row < visible; row++)
    {
        const int itemY = listY + row * m_ItemHeight;
        if (CheckMouseIn(m_X, itemY, rowWidth, m_ItemHeight))
            return m_ScrollOffset + row;
    }
    return -1;
}

bool CNewUIComboBox::IsMouseOverWidget() const
{
    if (CheckMouseIn(m_X, m_Y, m_Width, m_ItemHeight))
        return true;
    if (m_bOpen && CheckMouseIn(m_X, GetListY(), m_Width, GetListHeight()))
        return true;
    return false;
}

bool CNewUIComboBox::UpdateMouseEvent()
{
    // Mouse-wheel scrolling inside the open dropdown (consumed so it doesn't
    // leak to other handlers like the volume sliders).
    if (m_bOpen && IsScrollable() && MouseWheel != 0 &&
        CheckMouseIn(m_X, GetListY(), m_Width, GetListHeight()))
    {
        // MouseWheel sign convention matches NewUIOptionWindow sliders:
        // positive = wheel up = scroll toward earlier items.
        if (MouseWheel > 0) m_ScrollOffset--;
        else                m_ScrollOffset++;
        ClampScrollOffset();
        MouseWheel = 0;
    }

    if (!SEASON3B::IsPress(VK_LBUTTON))
        return false;

    const bool clickedClosedField = CheckMouseIn(m_X, m_Y, m_Width, m_ItemHeight);

    // Clicking the closed field toggles the dropdown.
    if (clickedClosedField)
    {
        m_bOpen = !m_bOpen;
        if (m_bOpen)
            ScrollToShowIndex(m_SelectedIndex);
        return false;
    }

    if (!m_bOpen)
        return false;

    // Dropdown open: did the click land on an item?
    const int hitIdx = GetItemIndexAtMouse();
    if (hitIdx < 0)
    {
        // Click landed outside the list (or on the scrollbar column) -- close
        // without changing selection.
        m_bOpen = false;
        return false;
    }

    const bool changed = (hitIdx != m_SelectedIndex);
    m_SelectedIndex = hitIdx;
    m_bOpen = false;
    return changed;
}

void CNewUIComboBox::RenderSkinnedField(bool hovered)
{
    const UI::NineSlice::StateTints& tints = m_pFieldSkin->tints;
    const unsigned int tint = m_bOpen ? tints.pressed : (hovered ? tints.hover : tints.normal);
    UI::NineSlice::Render(m_pFieldSkin->texture, static_cast<float>(m_X), static_cast<float>(m_Y),
        static_cast<float>(m_Width), static_cast<float>(m_ItemHeight), tint);
}

void CNewUIComboBox::RenderChevron()
{
    const int centreX = m_X + m_Width - CHEVRON_RIGHT_INSET;
    const int top = m_Y + (m_ItemHeight - CHEVRON_ROWS) / 2;
    for (int row = 0; row < CHEVRON_ROWS; ++row)
    {
        const int index = m_bOpen ? (CHEVRON_ROWS - 1 - row) : row;
        const int width = CHEVRON_TOP_WIDTH - index * CHEVRON_ROW_NARROW;
        DrawSolidRect(centreX - width / 2, top + row, width, 1, CHEVRON_BRIGHTNESS);
    }
}

void CNewUIComboBox::RenderClosedField()
{
    const bool hoverClosed = CheckMouseIn(m_X, m_Y, m_Width, m_ItemHeight);
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);

    if (m_pFieldSkin == nullptr)
    {
        const float closedBrightness = m_bOpen ? BG_BRIGHTNESS_OPEN
                                                : (hoverClosed ? BG_BRIGHTNESS_HOVER
                                                               : BG_BRIGHTNESS_IDLE);
        DrawSolidRect(m_X, m_Y, m_Width, m_ItemHeight, closedBrightness);

        g_pRenderText->SetTextColor(255, 255, 255, 255);
        g_pRenderText->RenderText(m_X + TEXT_PAD_X, m_Y + TEXT_PAD_Y, m_Labels[m_SelectedIndex]);

        // Dropdown arrow glyph (plain text -- no asset needed)
        g_pRenderText->SetTextColor(255, 230, 200, 255);
        g_pRenderText->RenderText(m_X + m_Width - ARROW_WIDTH, m_Y + TEXT_PAD_Y, m_bOpen ? L"^" : L"v");
        return;
    }

    RenderSkinnedField(hoverClosed);
    const SIZE textSize = g_pRenderText->MeasureText(m_Labels[m_SelectedIndex],
        static_cast<int>(wcslen(m_Labels[m_SelectedIndex])));
    g_pRenderText->SetTextColor(255, 255, 255, 255);
    g_pRenderText->RenderText(m_X + TEXT_PAD_X, m_Y + (m_ItemHeight - textSize.cy) / 2, m_Labels[m_SelectedIndex]);
    RenderChevron();
}

void CNewUIComboBox::Render()
{
    if (m_ItemCount <= 0 || m_Labels == nullptr)
        return;

    RenderClosedField();

    // --- Expanded list (drawn on top of anything below) ---
    if (!m_bOpen)
        return;

    const int listY = GetListY();
    const int visible = GetVisibleCount();
    const bool hasScrollbar = IsScrollable();
    const int rowWidth = hasScrollbar ? (m_Width - SCROLLBAR_WIDTH) : m_Width;
    const int rowInset = m_pFieldSkin != nullptr ? SKINNED_LIST_ROW_INSET : 0;

    if (m_pFieldSkin != nullptr)
    {
        // Starts one pixel up so its top ring coincides with the closed field's bottom ring.
        UI::NineSlice::Render(m_pFieldSkin->texture, static_cast<float>(m_X), static_cast<float>(listY - 1),
            static_cast<float>(m_Width), static_cast<float>(GetListHeight() + 2), m_pFieldSkin->tints.normal);
    }

    g_pRenderText->SetTextColor(255, 255, 255, 255);

    for (int row = 0; row < visible; row++)
    {
        const int absIdx = m_ScrollOffset + row;
        const int itemY = listY + row * m_ItemHeight;
        const bool hoverItem = CheckMouseIn(m_X, itemY, rowWidth, m_ItemHeight);
        const bool isSelected = (absIdx == m_SelectedIndex);

        float bright = BG_BRIGHTNESS_IDLE;
        if (hoverItem)      bright = BG_BRIGHTNESS_HOVER;
        else if (isSelected) bright = BG_BRIGHTNESS_OPEN;

        if (m_pFieldSkin == nullptr || hoverItem || isSelected)
        {
            DrawSolidRect(m_X + rowInset, itemY, rowWidth - 2 * rowInset, m_ItemHeight, bright);
        }
        g_pRenderText->RenderText(m_X + TEXT_PAD_X, itemY + TEXT_PAD_Y, m_Labels[absIdx]);
    }

    // --- Scrollbar (track + proportional thumb) ---
    if (hasScrollbar)
    {
        const int listHeight = GetListHeight();
        const int barX = m_X + m_Width - SCROLLBAR_WIDTH;

        // Track
        DrawSolidRect(barX, listY, SCROLLBAR_WIDTH, listHeight, SCROLLBAR_TRACK_BRIGHT);

        // Thumb: size proportional to (visible / total), position to (offset / maxOffset).
        int thumbHeight = (listHeight * visible) / m_ItemCount;
        if (thumbHeight < m_ItemHeight / 2)
            thumbHeight = m_ItemHeight / 2;

        const int maxOffset = GetMaxScrollOffset();
        const int thumbTravel = listHeight - thumbHeight;
        const int thumbY = listY + ((maxOffset > 0) ? (thumbTravel * m_ScrollOffset / maxOffset) : 0);

        DrawSolidRect(barX, thumbY, SCROLLBAR_WIDTH, thumbHeight, SCROLLBAR_THUMB_BRIGHT);
    }
}
