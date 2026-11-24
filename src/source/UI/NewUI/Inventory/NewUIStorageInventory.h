//*****************************************************************************
// File: NewUIStorageInventory.h
//*****************************************************************************

#if !defined(AFX_NEWUISTORAGEINVENTORY_H__BD790479_EDDE_4981_9B03_A12163A58D5D__INCLUDED_)
#define AFX_NEWUISTORAGEINVENTORY_H__BD790479_EDDE_4981_9B03_A12163A58D5D__INCLUDED_

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

namespace SEASON3B
{
    class CNewUIStorageInventory : public CNewUIObj
    {
    public:
        enum IMAGE_LIST
        {
            IMAGE_STORAGE_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK, // Reference
            IMAGE_STORAGE_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
            IMAGE_STORAGE_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
            IMAGE_STORAGE_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
            IMAGE_STORAGE_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
            IMAGE_STORAGE_EXPAND_BTN = CNewUIMyInventory::IMAGE_INVENTORY_EXPAND_BTN,

            IMAGE_STORAGE_BTN_INSERT_ZEN = BITMAP_INTERFACE_NEW_STORAGE_BEGIN,
            IMAGE_STORAGE_BTN_TAKE_ZEN = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 1,
            IMAGE_STORAGE_BTN_UNLOCK = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 2,
            IMAGE_STORAGE_BTN_LOCK = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 3,

            IMAGE_STORAGE_MONEY = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 4,

            IMAGE_STORAGE_VAULT_ARROW_L = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 5,
            IMAGE_STORAGE_VAULT_ARROW_R = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 6,

            IMAGE_STORAGE_VAULT_BTN_L = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 7,
            IMAGE_STORAGE_VAULT_BTN_R = BITMAP_INTERFACE_NEW_STORAGE_BEGIN + 8,
        };

    private:
        static constexpr float STORAGE_WIDTH = 190.0f;
        static constexpr float STORAGE_HEIGHT = 429.0f;

        // The switch buttons sit in the row of the round Zen / lock buttons (same top and height).
        static constexpr int VAULT_BTN_PREV_X = 127;
        static constexpr int VAULT_BTN_NEXT_X = 152;
        static constexpr int VAULT_BTN_Y = 391;
        static constexpr int VAULT_BTN_WIDTH = 24;
        static constexpr int VAULT_BTN_HEIGHT = 29;

        // The most Zen the storage holds (the server's MaximumVaultMoney), shown as a tooltip on the Zen box.
        static constexpr __int64 MAX_STORAGE_ZEN = 1'000'000'000'000LL;
        static constexpr int ZEN_BOX_X = 10;
        static constexpr int ZEN_BOX_Y = 342;
        static constexpr int ZEN_BOX_WIDTH = 170;
        static constexpr int ZEN_BOX_HEIGHT = 46;
        static constexpr DWORD VAULT_SWITCH_COOLDOWN_MS = 250;

        enum STORAGE_BUTTON
        {
            BTN_INSERT_ZEN = 0,
            BTN_TAKE_ZEN,
            BTN_LOCK,
            MAX_BTN
        };

        CNewUIManager* m_pNewUIMng;
        POINT					m_Pos;
        CNewUIButton			m_abtn[MAX_BTN];
        CNewUIButton			m_BtnExpand;
        CNewUIButton m_BtnVaultPrev;
        CNewUIButton m_BtnVaultNext;
        CNewUIInventoryCtrl* m_pNewInventoryCtrl;

        bool					m_bLock;
        bool					m_bCorrectPassword;

        int						m_nVaultNumber;
        int						m_nVaultCount;
        DWORD					m_dwLastVaultSwitchRequest;

        bool					m_bItemAutoMove;
        int						m_nBackupMouseX;
        int						m_nBackupMouseY;

        bool					m_bTakeZen;
        int						m_nBackupTakeZen;
        int						m_nBackupInvenIndex;
        int						m_nBackupSourceInvenIndex;

    public:
        CNewUIStorageInventory();
        virtual ~CNewUIStorageInventory();

        bool Create(CNewUIManager* pNewUIMng, int x, int y);
        void Release();

        void SetPos(int x, int y);

        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();

        float GetLayerDepth();	//. 2.2f

        CNewUIInventoryCtrl* GetInventoryCtrl() const;

        bool ProcessClosing();
        bool InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket);
        int FindEmptySlot(ITEM* pItemObj);

        bool IsStorageLocked() { return m_bLock; }
        bool IsCorrectPassword() { return m_bCorrectPassword; }
        bool IsItemAutoMove() { return m_bItemAutoMove; }

        void SetBackupTakeZen(int nZen);

        bool ProcessMyInvenItemAutoMove(CNewUIInventoryCtrl* sourceCtrl = nullptr);

        void SendRequestItemToMyInven(ITEM* pItemObj, int nStorageIndex, int nInvenIndex);

        void ProcessToReceiveVaultSwitched(int nVaultNumber, int nVaultCount);
        void ProcessToReceiveStorageStatus(BYTE byStatus);
        void ProcessToReceiveStorageItems(int nIndex, std::span<const BYTE> pbyItemPacket);
        void ProcessStorageItemAutoMoveSuccess();
        void ProcessStorageItemAutoMoveFailure();

        int GetPointedItemIndex();

        void SetItemAutoMove(bool bItemAutoMove, int nSourceInvenIndex = -1);
        void SendRequestItemToStorage(ITEM* pItemObj, int nInvenIndex, int nStorageIndex);
    private:
        void LoadImages();
        void UnloadImages();

        void RenderBackImage();
        void RenderText();
        void RenderVaultButtons();
        bool ProcessVaultButtons();
        void RenderMaxZenTip() const;

        void ChangeLockBtnImage();
        void DeleteAllItems();

        void LockStorage(bool bLock);
        void SetCorrectPassword(bool bCorrectPassword)
        {
            m_bCorrectPassword = bCorrectPassword;
        }

        void InitBackupItemInfo();
        int GetBackupTakeZen() { return m_nBackupTakeZen; }
        void SetBackupInvenIndex(int nInvenIndex);
        int GetBackupInvenIndex() { return m_nBackupInvenIndex; }

        void ProcessInventoryCtrl();
        bool ProcessBtns();
        void ProcessStorageItemAutoMove();
    };
}

#endif // !defined(AFX_NEWUISTORAGEINVENTORY_H__BD790479_EDDE_4981_9B03_A12163A58D5D__INCLUDED_)
