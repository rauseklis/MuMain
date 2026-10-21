// NewUIJewelBankWindow.h: the Jewel Bank window, opened with Ctrl+J.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <chrono>
#include <string>

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/JewelBank/JewelBankModel.h"

namespace SEASON3B
{
    // A per-account bank for the ten jewels and the five stat fruits. Two tabs, Deposit and Withdraw, both list every
    // kind with what the bank holds and what the player carries. A row is ticked to move everything available of the
    // kind, or given an amount; any combination of rows is moved with one button. The server is authoritative: the
    // window only sends requests and shows what the server answers.
    class CNewUIJewelBankWindow : public CNewUIObj, public INewUI3DRenderObj
    {
    public:
        enum TAB
        {
            TAB_DEPOSIT = 0,
            TAB_WITHDRAW,
            TAB_COUNT,
        };

        static constexpr int WINDOW_WIDTH = 360;
        static constexpr int WINDOW_HEIGHT = 414;

        CNewUIJewelBankWindow();
        ~CNewUIJewelBankWindow() override;

        bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y);
        void Release();

        // Called once when the window becomes visible.
        void OpeningProcess();
        // Called once when the window is hidden.
        void ClosingProcess();

        // Called from the network dispatch (WSclient.cpp, 0xD6).
        void SetState(const JewelBank::StateResponse& state);
        void SetOperationResponse(const JewelBank::OperationResponse& response);
        void SetPickupNotification(const JewelBank::PickupNotification& notification);

        bool Render() override;
        // The item models of the rows, drawn in the 3D pass after every 2D control (like the Auction House icons).
        void Render3D() override;
        bool IsVisible() const override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override;

    private:
        using Clock = std::chrono::steady_clock;

        void RenderFrame();
        void RenderTabs();
        void RenderHeader();
        void RenderRows();
        void RenderActions();
        void InitButton(CNewUIButton* button, const wchar_t* caption);
        void PositionChildren();
        bool BtnProcess();
        bool RowAt(int mouseX, int mouseY, int& row, bool& onAmount) const;
        JewelBank::Available CurrentAvailable() const;
        JewelBank::Available AvailableFor(int tab) const;
        JewelBank::Amounts& CurrentAmounts();
        void SwitchTab(int tab);
        void BeginEdit(int row);
        void CommitEdit();
        void CancelEdit();
        void SendOpenRequest();
        void SendAction();
        void SendAutoDeposit(bool enabled);
        void BeginOperation(JewelBank::Operation operation);
        void ShowMessage(const std::wstring& text, bool good);
        void RefreshButtons();

        CNewUIManager* m_pNewUIMng;
        CNewUI3DRenderMng* m_pNewUI3DRenderMng;
        POINT m_Pos;

        bool m_bDragging;
        int m_iDragGrabOffsetX;
        int m_iDragGrabOffsetY;

        CNewUIButton m_BtnSelectAll;
        CNewUIButton m_BtnClear;
        CNewUIButton m_BtnAction;
        CUITextInputBox m_AmountInput;
        int m_iEditRow;

        int m_iTab;
        std::array<JewelBank::Amounts, TAB_COUNT> m_Amounts;
        std::array<uint64_t, JewelBank::KIND_COUNT> m_Counts;
        std::array<uint32_t, JewelBank::KIND_COUNT> m_Inventory;
        bool m_bHasState;
        bool m_bAutoDeposit;

        bool m_bOperationPending;
        std::array<uint8_t, 16> m_PendingOperationId;
        JewelBank::Operation m_PendingOperation;
        uint64_t m_PendingRequested;
        Clock::time_point m_OperationDeadline;
        Clock::time_point m_NextPoll;

        std::wstring m_Message;
        bool m_bMessageGood;
    };
} // namespace SEASON3B
