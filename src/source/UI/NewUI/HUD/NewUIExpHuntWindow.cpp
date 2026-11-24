#include "stdafx.h"
#include "UI/NewUI/HUD/NewUIExpHuntWindow.h"

#include "Core/Globals/_enum.h"
#include "GameLogic/ExpHunt/ExpHuntTracker.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/HUD/ExpHuntBarLayout.h"
#include "UI/Scaling/UITransform.h"

#include <algorithm>
#include <string>

namespace SEASON3B
{
    namespace
    {
    namespace Bar = UI::HUD::ExpHuntBar;

    // The widest line the bar must hold without clipping: the largest value the server can report.
    constexpr wchar_t kWidestText[] = L"Experience Hunter: 9,999,999,999 / min";

    // Draws a piece of the atlas so that its edges fall on whole screen pixels and the texture is sampled exactly
    // (no half-texel inset), which keeps the art as sharp as it was drawn.
    void RenderAtlasRect(int image, const Bar::Rect& destination, const Bar::Rect& source)
    {
        const UI::HUD::TopLeftBar::SnappedRect snapped = UI::HUD::TopLeftBar::SnapRect(
            destination, g_fScreenRate_x, g_fScreenRate_y, g_fScreenOffset_x, g_fScreenOffset_y);
        RenderImage(image, snapped.x, snapped.y, snapped.width, snapped.height, Bar::AtlasUv(source.x),
                    Bar::AtlasUv(source.y), Bar::AtlasUv(source.width), Bar::AtlasUv(source.height),
                    RGBA(255, 255, 255, 255));
    }

        std::wstring FormatExpRate(double ratePerMinute)
        {
            std::uint64_t whole = ratePerMinute > 0.0 ? static_cast<std::uint64_t>(ratePerMinute + 0.5) : 0;

            std::wstring digits = std::to_wstring(whole);
            std::wstring grouped;
            const int firstGroupLen = static_cast<int>(digits.size() % 3 == 0 ? 3 : digits.size() % 3);

            grouped.append(digits, 0, firstGroupLen);
            for (size_t i = firstGroupLen; i < digits.size(); i += 3)
            {
                grouped += L',';
                grouped.append(digits, i, 3);
            }

            return L"Experience Hunter: " + grouped + L" / min";
        }
    }

    CNewUIExpHuntWindow::CNewUIExpHuntWindow()
        : m_pNewUIMng(nullptr), m_Pos{0, 0}, m_iWidth(Bar::kMinWidth), m_bMoved(false), m_bDragging(false),
          m_iDragGrabOffsetX(0), m_iDragGrabOffsetY(0)
    {
    }

    CNewUIExpHuntWindow::~CNewUIExpHuntWindow()
    {
        Release();
    }

    bool CNewUIExpHuntWindow::Create(CNewUIManager* pNewUIMng)
    {
        if (nullptr == pNewUIMng)
        {
            return false;
        }

        m_pNewUIMng = pNewUIMng;
        ResetToDefaultPosition();
        m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_EXPHUNT, this);
        LoadImages();
        Show(false);
        return true;
    }

    void CNewUIExpHuntWindow::ResetToDefaultPosition()
    {
        m_bMoved = false;
        m_Pos.y = 0;
        m_bDragging = false;
        m_iDragGrabOffsetX = 0;
        m_iDragGrabOffsetY = 0;
    }

    void CNewUIExpHuntWindow::Release()
    {
        if (m_pNewUIMng)
        {
            UnloadImages();
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
    }

    void CNewUIExpHuntWindow::LoadImages()
    {
        LoadBitmap(L"Interface\\ExpHuntBar\\ExpHuntBar.tga", IMAGE_EXPHUNT_ATLAS, GL_LINEAR);
    }

    void CNewUIExpHuntWindow::UnloadImages()
    {
        DeleteBitmap(IMAGE_EXPHUNT_ATLAS);
    }

    bool CNewUIExpHuntWindow::UpdateMouseEvent()
    {
        RefreshLayout();
        const bool hovering = CheckMouseIn(m_Pos.x, m_Pos.y, m_iWidth, Bar::kHitHeight);

        if (!m_bDragging && hovering && IsPress(VK_LBUTTON))
        {
            m_bDragging = true;
            m_iDragGrabOffsetX = MouseX - m_Pos.x;
            m_iDragGrabOffsetY = MouseY - m_Pos.y;
        }

        if (m_bDragging)
        {
            // MouseLButton (not MouseLButtonPush) is required here: Push is a
            // one-shot press-edge flag cleared every frame regardless of
            // whether the button is still held, so using it to decide whether
            // to KEEP dragging ended the drag one frame after every press.
            if (!MouseLButton)
            {
                m_bDragging = false;
                return false;
            }

            m_bMoved = true;
            m_Pos.x = MouseX - m_iDragGrabOffsetX;
            m_Pos.y = MouseY - m_iDragGrabOffsetY;

            return false;
        }

        return !hovering;
    }

    bool CNewUIExpHuntWindow::UpdateKeyEvent()
    {
        return true;
    }

    bool CNewUIExpHuntWindow::Update()
    {
        return true;
    }

    // The text decides the width: the bar is as wide as the longest line it has to hold (never narrower than the widest
    // value the server can report) so nothing is clipped or touches the ends.
    void CNewUIExpHuntWindow::RefreshLayout()
    {
        m_Text = FormatExpRate(GameLogic::ExpHunt::GetRatePerMinute());

        g_pRenderText->SetFont(g_hFontBold);
        const int current = g_pRenderText->MeasureText(m_Text.c_str(), static_cast<int>(m_Text.size())).cx;
        const int widest = g_pRenderText->MeasureText(kWidestText, static_cast<int>(wcslen(kWidestText))).cx;
        g_pRenderText->SetFont(g_hFont);

        m_iWidth = Bar::BarWidth(std::max(current, widest));

        if (!m_bMoved)
        {
            const auto bounds = UI::Scaling::FloatingWorkspaceBounds(WindowWidth, WindowHeight);
            m_Pos.x = bounds.width - m_iWidth;
        }
    }

    bool CNewUIExpHuntWindow::Render()
    {
        RefreshLayout();
        EnableAlphaTest();

        const auto draw = [this](const Bar::Rect& piece, const Bar::Rect& source)
        {
            RenderAtlasRect(IMAGE_EXPHUNT_ATLAS,
                            Bar::Rect{m_Pos.x + piece.x, m_Pos.y + piece.y, piece.width, piece.height}, source);
        };
        draw(Bar::LeftPiece(), Bar::kLeftSource);
        draw(Bar::MiddlePiece(m_iWidth), Bar::kMiddleSource);
        draw(Bar::RightPiece(m_iWidth), Bar::kRightSource);

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(255, 220, 120, 255);
        const Bar::Rect box = Bar::TextBox(m_iWidth);
        const int textHeight = g_pRenderText->MeasureText(m_Text.c_str(), static_cast<int>(m_Text.size())).cy;
        g_pRenderText->RenderText(m_Pos.x + box.x, m_Pos.y + Bar::kTextCentreY - textHeight / 2, m_Text.c_str(),
                                  box.width, 0, RT3_SORT_CENTER);
        g_pRenderText->SetFont(g_hFont);

        DisableAlphaBlend();

        return true;
    }

    float CNewUIExpHuntWindow::GetLayerDepth()
    {
        // Must be above MiniMap (8.1f) and MoveCommandWindow (8.3f): those windows'
        // UpdateMouseEvent() unconditionally claim almost the entire screen for their
        // own broad mouse checks and, being higher depth, are dispatched mouse events
        // first. A lower depth here meant this window's own UpdateMouseEvent() never
        // ran at all while the mouse was anywhere near the top of the screen, so
        // dragging could never even start. Confirmed via CNewUIManager::UpdateMouseEvent()
        // (descending-depth dispatch, hard-stops on the first false) and
        // CNewUIMiniMap::UpdateMouseEvent()'s unconditional CheckMouseIn(0,0,640,430) claim.
        return 8.5f;
    }
}
