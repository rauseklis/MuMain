// Small draggable HUD window for the Experience Hunter: shows the trailing
// experience-per-minute rate from GameLogic::ExpHunt.
#pragma once

#include "UI/NewUI/NewUIManager.h"

namespace SEASON3B
{
    class CNewUIExpHuntWindow : public CNewUIObj
    {
    public:
        CNewUIExpHuntWindow();
        ~CNewUIExpHuntWindow() override;

        bool Create(CNewUIManager* pNewUIMng);
        void Release();
        void ResetToDefaultPosition();

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

        static constexpr int WND_WIDTH = 220;
        static constexpr int WND_HEIGHT = 20;
        static constexpr int CAP_WIDTH = 18;
        static constexpr int DEFAULT_X = 420;
        static constexpr int DEFAULT_Y = 0;

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        bool m_bDragging;
        int m_iDragGrabOffsetX;
        int m_iDragGrabOffsetY;
    };
}
