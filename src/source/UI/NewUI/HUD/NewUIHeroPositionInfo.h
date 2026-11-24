// NewUIHeroPositionInfo.h: interface for the CNewUIHeroPositionInfo class.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEWUIHEROPOSITIONINFO_H__5D452B62_B315_41EE_A862_5929286925F7__INCLUDED_)
#define AFX_NEWUIHEROPOSITIONINFO_H__5D452B62_B315_41EE_A862_5929286925F7__INCLUDED_

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/HUD/TopLeftBarLayout.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    class CNewUIHeroPositionInfo : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_HERO_POSITION_INFO_ATLAS = BITMAP_HERO_POSITION_INFO_BEGIN, // TopLeftBar.tga (128, 128)
        };

    private:
        CNewUIManager* m_pNewUIMng;
        POINT            m_Pos;
        POINT            m_CurHeroPosition;
        // The buttons only keep the hit area, hover/pressed state and tooltip; the faces are drawn from the atlas.
        CNewUIButton     m_BtnConfig;
        CNewUIButton     m_BtnStart;
        CNewUIButton     m_BtnStop;
        CNewUIButton m_BtnExpHunt;

    public:
        CNewUIHeroPositionInfo();
        virtual ~CNewUIHeroPositionInfo();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        void SetPos(int x, int y);

        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();

        bool BtnProcess();

        float GetLayerDepth();	//. 4.3f

        void OpenningProcess();
        void ClosingProcess();

        void SetCurHeroPosition(int x, int y);
    private:
        bool ProcessExpHuntButton();
        void PlaceButtons();
        void RenderFrame();
        void RenderTexts();
        void RenderButtonFace(CNewUIButton& button, UI::HUD::TopLeftBar::Button which, UI::HUD::TopLeftBar::Face face);
        void RenderAtlasRect(const UI::HUD::TopLeftBar::Rect& destination, const UI::HUD::TopLeftBar::Rect& source);
        void LoadImages();
        void UnloadImages();
    };
}

#endif // !defined(AFX_NEWUIHEROPOSITIONINFO_H__5D452B62_B315_41EE_A862_5929286925F7__INCLUDED_)
