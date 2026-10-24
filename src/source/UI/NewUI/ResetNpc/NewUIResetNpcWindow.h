// NewUIResetNpcWindow.h: the window of Raymond the Reset Master, the NPC which resets characters.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <chrono>

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/ResetNpc/ResetNpcModel.h"

namespace SEASON3B
{
    // Shows what the next reset costs (level, Zen and fruits from the Jewel Bank) next to what the character has, and a button
    // which asks for the reset. The server decides: the window only shows what the server answers (0xD7 sub 0x80) and sends
    // the request of the button. Pressing the button twice (the second press within a few seconds) avoids resetting by accident.
    class CNewUIResetNpcWindow : public CNewUIObj
    {
    public:
        static constexpr int WINDOW_WIDTH = 360;
        static constexpr int WINDOW_HEIGHT = 290;

        CNewUIResetNpcWindow();
        ~CNewUIResetNpcWindow() override;

        bool Create(CNewUIManager* pNewUIMng);
        void Release();

        // Called once when the window becomes visible.
        void OpeningProcess();
        // Called once when the window is hidden: lets the server know that the dialog with the NPC is over.
        void ClosingProcess();

        // Called from the network dispatch (WSclient.cpp, 0xD7).
        void SetState(const ResetNpc::State& state);

        bool Render() override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override;

    private:
        struct Rect
        {
            int x;
            int y;
            int width;
            int height;
        };

        [[nodiscard]] Rect ButtonRect() const;
        [[nodiscard]] Rect CloseRect() const;
        void RenderRequirementRow(int row, const wchar_t* label, const std::wstring& value, bool met) const;
        void PressResetButton();

        CNewUIManager* m_pNewUIMng = nullptr;
        int m_posX = 0;
        int m_posY = 0;
        ResetNpc::State m_state;
        bool m_hasState = false;
        bool m_waitingForAnswer = false;
        bool m_confirmPending = false;
        std::chrono::steady_clock::time_point m_confirmDeadline;
        const wchar_t* m_message = L"";
    };
}
