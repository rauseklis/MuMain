// Small draggable HUD window for the /hunt command: shows the trailing
// experience-per-minute rate from GameLogic::ExpHunt. No background art -
// renders as a flat color box with text, the same way the debug overlays do.
#pragma once

#include "UI/NewUI/NewUIManager.h"

namespace SEASON3B
{
    class CNewUIExpHuntWindow : public CNewUIObj
    {
    public:
        CNewUIExpHuntWindow();
        ~CNewUIExpHuntWindow() override;

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        bool Update() override;
        bool Render() override;

        float GetLayerDepth() override;

    private:
        enum IMAGE_LIST
        {
            IMAGE_EXPHUNT_CAP = BITMAP_EXPHUNT_BEGIN,
            IMAGE_EXPHUNT_MIDDLE,
        };

        void LoadImages();
        void UnloadImages();

        static constexpr int WND_WIDTH = 280;
        static constexpr int WND_HEIGHT = 25;
        static constexpr int CAP_WIDTH = 22;

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        bool m_bDragging;
        int m_iDragGrabOffsetX;
        int m_iDragGrabOffsetY;
    };
}
