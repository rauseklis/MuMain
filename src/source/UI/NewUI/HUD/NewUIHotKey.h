#ifndef _NEWUIHOTKEY_H_
#define _NEWUIHOTKEY_H_

#pragma once

#include "UI/NewUI/NewUIBase.h"

namespace SEASON3B
{
    class CNewUIManager;

    class CNewUIHotKey : public CNewUIObj
    {
        CNewUIManager* m_pNewUIMng;
        bool			m_bStateGameOver;
        bool m_bLogoutPending = false;
        int m_nLogoutType = 0;
        int m_nLogoutSecondsShown = 0;
        unsigned long m_dwLogoutStartTime = 0;

    public:
        CNewUIHotKey();
        virtual ~CNewUIHotKey();

        bool Create(CNewUIManager* pNewUIMng);
        void Release();

        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();

        float GetLayerDepth();	//. 1.0f
        float GetKeyEventOrder();	//. 1.0f

        bool CanUpdateKeyEvent();
        bool CanUpdateKeyEventRelatedMyInventory();

        void SetStateGameOver(bool bGameOver);	// 게임오버중인 상태
        bool IsStateGameOver();

        /// Starts the delayed log out (exit game / server selection / character selection).
        /// The log out is sent once the countdown has run out.
        void StartLogoutCountdown(int logoutType);
        void CancelLogoutCountdown();

        bool AutoGetItem();

    private:
        void ResetMouseRButton();
    };
}

#endif	// _NEWUIHOTKEY_H_