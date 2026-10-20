// NewUIJewelBankWindow.h: the Jewel Bank window, opened with Ctrl+J.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <string>

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/JewelBank/JewelBankModel.h"

namespace SEASON3B
{
    // A per-account bank for the ten jewels and the five stat fruits. It lists every kind with the stored count;
    // the player picks a row, types an amount and withdraws it as items, deposits the kind or everything the bank
    // accepts from the inventory, and chooses whether picked-up jewels and fruits go to the bank automatically.
    // The server is authoritative: the window only sends requests and shows what the server answers.
    class CNewUIJewelBankWindow : public CNewUIObj
    {
    public:
        static constexpr int WINDOW_WIDTH = 330;
        static constexpr int WINDOW_HEIGHT = 446;

        CNewUIJewelBankWindow();
        ~CNewUIJewelBankWindow() override;

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        // Called once when the window becomes visible: asks the server for the current counts.
        void OpeningProcess();
        // Called once when the window is hidden.
        void ClosingProcess();

        // Called from the network dispatch (WSclient.cpp, 0xD6).
        void SetState(const JewelBank::StateResponse& state);
        void SetOperationResponse(const JewelBank::OperationResponse& response);
        void SetPickupNotification(const JewelBank::PickupNotification& notification);

        bool Render() override;
        bool IsVisible() const override;
        bool Update() override;
        bool UpdateMouseEvent() override;
        bool UpdateKeyEvent() override;
        float GetLayerDepth() override;

    private:
        void RenderFrame();
        void RenderRows();
        void RenderActionPanel();
        void InitButton(CNewUIButton* button, int x, int y, int width, const wchar_t* caption);
        void PositionChildren();
        bool BtnProcess();
        bool RowAt(int mouseX, int mouseY, int& index) const;
        uint64_t SelectedCount() const;
        void SendOpenRequest();
        void SendDeposit(uint8_t kindCode);
        void SendWithdraw();
        void SendAutoDeposit(bool enabled);
        void BeginOperation(JewelBank::Operation operation);
        void ShowMessage(const std::wstring& text, bool good);
        void RefreshButtons();

        CNewUIManager* m_pNewUIMng;
        POINT m_Pos;

        bool m_bDragging;
        int m_iDragGrabOffsetX;
        int m_iDragGrabOffsetY;

        CNewUIButton m_BtnClose;
        CNewUIButton m_BtnWithdraw;
        CNewUIButton m_BtnMax;
        CNewUIButton m_BtnDeposit;
        CNewUIButton m_BtnDepositAll;
        CUITextInputBox m_AmountInput;

        std::array<uint64_t, JewelBank::KIND_COUNT> m_Counts;
        bool m_bHasState;
        bool m_bAutoDeposit;
        int m_iSelected;

        bool m_bOperationPending;
        std::array<uint8_t, 16> m_PendingOperationId;
        JewelBank::Operation m_PendingOperation;

        std::wstring m_Message;
        bool m_bMessageGood;
    };
} // namespace SEASON3B
