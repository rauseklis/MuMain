// NewUIAuctionSellPanel.h: the form of the Auction House's Sell tab.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Widgets/NewUIComboBox.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/AuctionHouse/AuctionModel.h"

typedef struct tagITEM ITEM;

namespace SEASON3B
{
    // The Sell tab below the item the player picked in the inventory: how to sell it (Auction or Buy Now), the
    // price as rows of (currency, amount) - an auction row also has an optional buyout - the duration, an optional
    // note, and the flat tax the server holds.
    // The panel owns its controls and turns them into a SellDraft; the window sends the request and reports the
    // outcome back with ShowResult(). It draws with the Auction House's shared chrome and knows nothing of packets.
    class CNewUIAuctionSellPanel
    {
    public:
        static constexpr int MaxRows = AuctionHouse::MaximumSellComponents;
        static constexpr int KindCount = static_cast<int>(AuctionHouse::PriceKind::CommandFruit) + 1;
        static constexpr int DurationChoices = 4;

        CNewUIAuctionSellPanel();

        // Creates the controls for a window at the given position. mixedSupported says whether the server takes mixed
        // prices (a Buy Now listing, or an auction in several currencies); until the Open response says it does, those
        // are shown but Create stays disabled for them, with a note.
        void Create(int windowX, int windowY, bool mixedSupported);

        // Applies the Open response's mixed-price switch and the flat listing tax in Zen (0 = none).
        void SetMixedSupported(bool mixedSupported);
        void SetSaleFee(uint32_t saleFee);
        void SetPos(int windowX, int windowY);

        // Shows the form's text inputs while the Sell tab is up and hides them (dropping focus) otherwise.
        void SetActive(bool active);

        // Applies the Open response's settings: the fee rates and which durations the server offers.
        void SetServerSettings(uint16_t listingFeeBasisPoints, uint16_t successFeeBasisPoints, uint8_t durationMask,
            uint8_t defaultDurationHours);

        // While a request is in flight Create is disabled so a click cannot send a second listing.
        void SetBusy(bool busy);

        // The outcome of a create request. Success empties the form for the next listing.
        void ShowResult(bool success, const std::wstring& message);

        // Resets mode, prices, duration and note.
        void ClearForm();

        // Handles the mouse for the form's controls. Returns true when the press or release belonged to one of them.
        bool UpdateMouseEvent();

        // Text-input focus, for the window's key routing: Enter submits, Escape drops focus.
        bool HasInputFocus() const;
        HWND FocusedInputHandle() const;
        void ReleaseInputFocus();

        // Whether the player has picked an item to sell; Create needs one.
        void SetItemSelected(bool hasItem);

        // Presses Create as if the button had been clicked (Enter in a field). Does nothing unless the form is valid.
        void RequestCreate();

        // True once after Create was pressed on a valid form; fills the draft and the note.
        bool TakeCreateRequest(AuctionHouse::SellDraft& draft, std::wstring& note);

        // 2D drawing. item is the player's selected item or null; its slot, name and hint are drawn in the card.
        void Render(const ITEM* item);

        // Geometry of the selected item's icon (window coordinates), for the 3D pass and its tooltip.
        void GetItemIconRect(int& x, int& y, int& size) const;
        bool IsMouseOverItemIcon() const;

    private:
        void LayoutControls();
        void RefreshVisibility();
        void SetMode(AuctionHouse::SellMode mode);
        void AddRow();
        void RemoveRow(int row);
        void KeepKindsUnique(int changedRow);
        AuctionHouse::SellDraft BuildDraft();
        std::wstring StatusText(AuctionHouse::SellDraftIssue issue) const;
        int64_t ParseQuantity(CUITextInputBox& input) const;
        int SelectedDurationHours() const;
        void RenderItemCard(const ITEM* item);
        void RenderModeButtons();
        void RenderPricePanel();
        void RenderOptions();
        void RenderFees(const AuctionHouse::SellDraft& draft);
        void RenderField(int x, int y, int width, bool focused) const;
        void InitButton(CNewUIButton& button, int x, int y, int width, const wchar_t* caption);
        bool AnyInputFocused() const;
        bool IsMouseOverAnyField() const;

        POINT m_Pos;
        bool m_bActive;
        bool m_bBuyNowSupported;
        uint32_t m_SaleFee;
        bool m_bBusy;
        bool m_bHasItem;
        bool m_bCreateRequested;
        bool m_bHasResult;
        bool m_bResultSuccess;
        std::wstring m_ResultMessage;

        AuctionHouse::SellMode m_Mode;
        int m_RowCount;
        uint16_t m_ListingFeeBasisPoints;
        uint16_t m_SuccessFeeBasisPoints;

        // One kind per price row (the auction uses row 0), the amount typed beside it, and the auction's optional buyout.
        const wchar_t* m_KindLabels[KindCount];
        // The kind each row last had selected, to swap kinds when a pick duplicates another row's.
        int m_KindIndex[MaxRows];
        CNewUIComboBox m_KindCombos[MaxRows];
        CUITextInputBox m_QuantityInputs[MaxRows];
        // The auction's optional buyout beside each row's starting amount.
        CUITextInputBox m_BuyoutInputs[MaxRows];

        const wchar_t* m_DurationLabels[DurationChoices];
        int m_DurationHours[DurationChoices];
        int m_DurationCount;
        CNewUIComboBox m_DurationCombo;
        CUITextInputBox m_NoteInput;

        CNewUIButton m_BtnAdd;
        CNewUIButton m_BtnCreate;
        CNewUIButton m_BtnClear;
    };
}
