// Small draggable HUD window for the Experience Hunter: shows the trailing
// experience-per-minute rate from GameLogic::ExpHunt.
#pragma once

#include "UI/NewUI/NewUIManager.h"

#include <string>

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
            IMAGE_EXPHUNT_ATLAS = BITMAP_EXPHUNT_BEGIN,
        };

        void LoadImages();
        void UnloadImages();

        // Measures the text and sizes the bar to it; until the bar is dragged it stays in the top-right corner.
        void RefreshLayout();

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;
        std::wstring m_Text;
        int m_iWidth;
        bool m_bMoved;
        bool m_bDragging;
        int m_iDragGrabOffsetX;
        int m_iDragGrabOffsetY;
    };
}
