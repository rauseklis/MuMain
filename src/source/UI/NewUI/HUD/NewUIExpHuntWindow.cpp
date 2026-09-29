#include "stdafx.h"
#include "UI/NewUI/HUD/NewUIExpHuntWindow.h"

#include "Core/Globals/_enum.h"
#include "GameLogic/ExpHunt/ExpHuntTracker.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/Legacy/UIControls.h"

#include <string>

namespace SEASON3B
{
    namespace
    {
        constexpr float CAP_SOURCE_WIDTH = 22.f;
        constexpr float CAP_SOURCE_HEIGHT = 25.f;
        constexpr float TEXTURE_SIZE = 32.f;
        constexpr int TEXT_TOP_OFFSET = 3;

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
        : m_pNewUIMng(nullptr), m_Pos{ 0, 0 }, m_bDragging(false),
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
        m_Pos.x = DEFAULT_X;
        m_Pos.y = DEFAULT_Y;
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
        LoadBitmap(L"Interface\\Minimap_positionA.tga", IMAGE_EXPHUNT_CAP, GL_LINEAR);
        LoadBitmap(L"Interface\\Minimap_positionB.tga", IMAGE_EXPHUNT_MIDDLE, GL_LINEAR);
    }

    void CNewUIExpHuntWindow::UnloadImages()
    {
        DeleteBitmap(IMAGE_EXPHUNT_CAP);
        DeleteBitmap(IMAGE_EXPHUNT_MIDDLE);
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
            // MouseLButton (not MouseLButtonPush) is required here: Push is a
            // one-shot press-edge flag cleared every frame regardless of
            // whether the button is still held, so using it to decide whether
            // to KEEP dragging ended the drag one frame after every press.
            if (!MouseLButton)
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

        const float middleWidth = static_cast<float>(WND_WIDTH - (CAP_WIDTH * 2));
        const float capU = 0.5f / TEXTURE_SIZE;
        const float capV = 0.5f / TEXTURE_SIZE;
        const float capUWidth = (CAP_SOURCE_WIDTH - 1.f) / TEXTURE_SIZE;
        const float capVHeight = (CAP_SOURCE_HEIGHT - 1.f) / TEXTURE_SIZE;

        RenderImage(IMAGE_EXPHUNT_CAP, static_cast<float>(m_Pos.x), static_cast<float>(m_Pos.y),
            static_cast<float>(CAP_WIDTH), static_cast<float>(WND_HEIGHT),
            capU, capV, capUWidth, capVHeight);

        RenderImage(IMAGE_EXPHUNT_MIDDLE, static_cast<float>(m_Pos.x + CAP_WIDTH), static_cast<float>(m_Pos.y),
            middleWidth, static_cast<float>(WND_HEIGHT), 0.1f, 0.f, 22.4f / 32.f, 25.f / 32.f);

        const float rightCapU = capU + capUWidth;
        RenderImage(IMAGE_EXPHUNT_CAP, static_cast<float>(m_Pos.x + WND_WIDTH - CAP_WIDTH),
            static_cast<float>(m_Pos.y), static_cast<float>(CAP_WIDTH), static_cast<float>(WND_HEIGHT),
            rightCapU, capV, -capUWidth, capVHeight);

        const std::wstring text = FormatExpRate(GameLogic::ExpHunt::GetRatePerMinute());

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0);
        g_pRenderText->SetTextColor(255, 220, 120, 255);
        g_pRenderText->RenderText(m_Pos.x + CAP_WIDTH, m_Pos.y + TEXT_TOP_OFFSET, text.c_str(),
            static_cast<int>(middleWidth), 0, RT3_SORT_CENTER);
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
