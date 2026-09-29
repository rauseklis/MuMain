#include "stdafx.h"
#include "UI/NewUI/HUD/NewUIExpHuntWindow.h"

#include "Core/Globals/_enum.h"
#include "GameLogic/ExpHunt/ExpHuntTracker.h"
#include "UI/Legacy/UIControls.h"

#include <string>

namespace SEASON3B
{
    namespace
    {
        std::wstring FormatExpRate(double ratePerMinute)
        {
            // Clamp non-positive rates to 0 - GetRatePerMinute() shouldn't return
            // negative values, but this guards against any rounding/edge-case
            // artifacts feeding a negative or NaN-derived value into the cast below.
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

            return grouped + L" / min";
        }
    }

    CNewUIExpHuntWindow::CNewUIExpHuntWindow()
        : m_pNewUIMng(nullptr), m_Pos{ 0, 0 }, m_bDragging(false),
          m_iDragGrabOffsetX(0), m_iDragGrabOffsetY(0)
    {
    }

    CNewUIExpHuntWindow::~CNewUIExpHuntWindow()
    {
        Release();
    }

    bool CNewUIExpHuntWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
    {
        if (nullptr == pNewUIMng)
        {
            return false;
        }

        m_pNewUIMng = pNewUIMng;
        m_Pos.x = x;
        m_Pos.y = y;
        m_pNewUIMng->AddUIObj(SEASON3B::INTERFACE_EXPHUNT, this);
        Show(false);
        return true;
    }

    void CNewUIExpHuntWindow::Release()
    {
        if (m_pNewUIMng)
        {
            m_pNewUIMng->RemoveUIObj(this);
            m_pNewUIMng = nullptr;
        }
    }

    bool CNewUIExpHuntWindow::UpdateMouseEvent()
    {
        const bool hovering = CheckMouseIn(m_Pos.x, m_Pos.y, WND_WIDTH, WND_HEIGHT);

        if (!m_bDragging && hovering && IsPress(VK_LBUTTON))
        {
            m_bDragging = true;
            m_iDragGrabOffsetX = MouseX - m_Pos.x;
            m_iDragGrabOffsetY = MouseY - m_Pos.y;
        }

        if (m_bDragging)
        {
            if (!MouseLButtonPush)
            {
                m_bDragging = false;
                return false;
            }

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

    bool CNewUIExpHuntWindow::Render()
    {
        EnableAlphaTest();

        RenderColorQuadARGB(static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y),
            static_cast<float>(WND_WIDTH), static_cast<float>(WND_HEIGHT), 0xB0000000u);

        const std::wstring text = FormatExpRate(GameLogic::ExpHunt::GetRatePerMinute());

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(255, 220, 120, 255);
        g_pRenderText->RenderText(m_Pos.x + 6, m_Pos.y + 4, text.c_str());
        g_pRenderText->SetFont(g_hFont);

        DisableAlphaBlend();

        return true;
    }

    float CNewUIExpHuntWindow::GetLayerDepth()
    {
        // Just above the chat/system log windows (6.1f / 6.05f) so this HUD
        // renders on top of them, but below the minimap and move-command
        // window (8.1f / 8.3f) so dragging this one never occludes those.
        return 6.5f;
    }
}
