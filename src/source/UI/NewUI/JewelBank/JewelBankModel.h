#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

// The pure side of the Jewel Bank window: the kinds the bank stores, the server's result codes with their
// messages, the parsers of the three server packets (0xD6 sub 0x80 to 0x82, all C1-with-subcode) and the
// amount helpers. No window state, no drawing, so it is unit tested without the game.
namespace JewelBank
{
    // The kinds are stored by code 1 to 15, in this order: ten jewels, then the five stat fruits. The codes are the
    // server's (JewelBankKinds) and are never renumbered.
    constexpr int KIND_COUNT = 15;
    constexpr uint8_t FIRST_KIND_CODE = 1;

    struct KindInfo
    {
        uint8_t Code;
        const wchar_t* Name;
        // ARGB colour of the small marker drawn in front of the name; jewels and fruits have no icon of their own here.
        uint32_t Color;
    };

    // The kind at a list position (0 to 14); the position is code - 1.
    [[nodiscard]] const KindInfo& KindAt(int index);
    // The kind with a wire code, or null for an unknown code.
    [[nodiscard]] const KindInfo* FindKind(uint8_t code);

    // The server's result codes (JewelBankResult).
    enum class Result : uint8_t
    {
        Success = 0,
        InvalidRequest = 1,
        InsufficientBalance = 2,
        InventoryFull = 3,
        CapReached = 4,
        NothingToDeposit = 5,
        NotConfigured = 6,
        StateRestricted = 7,
        RateLimited = 8,
        ServiceUnavailable = 9,
    };

    // The kinds of operation an operation response can answer.
    enum class Operation : uint8_t
    {
        Deposit = 0,
        Withdraw = 1,
        SetAutoDeposit = 2,
    };

    // A sentence for the player: what the result means and, for a refusal, what to do about it.
    [[nodiscard]] const wchar_t* ResultText(Result result);

    // The stored counts of the account (wire 0xD6 sub 0x80): Result@4, AutoDeposit@5, Count1..Count15 as u64 @6 + 8 * i.
    struct StateResponse
    {
        static constexpr size_t FullWireLength = 126;

        JewelBank::Result Result = JewelBank::Result::Success;
        bool AutoDeposit = false;
        std::array<uint64_t, KIND_COUNT> Counts{};

        [[nodiscard]] static std::optional<StateResponse> Parse(std::span<const uint8_t> packet);
    };

    // The outcome of a deposit, withdrawal or setting change (wire 0xD6 sub 0x81): OperationId(16)@4, Operation@20,
    // Result@21, KindCode@22, Units(u32)@23.
    struct OperationResponse
    {
        static constexpr size_t FullWireLength = 27;

        std::array<uint8_t, 16> OperationId{};
        JewelBank::Operation Operation = JewelBank::Operation::Deposit;
        JewelBank::Result Result = JewelBank::Result::Success;
        uint8_t KindCode = 0;
        uint32_t Units = 0;

        [[nodiscard]] static std::optional<OperationResponse> Parse(std::span<const uint8_t> packet);
    };

    // A picked-up item that went straight into the bank (wire 0xD6 sub 0x82): KindCode@4, Units(u32)@5, NewCount(u64)@9.
    struct PickupNotification
    {
        static constexpr size_t FullWireLength = 17;

        uint8_t KindCode = 0;
        uint32_t Units = 0;
        uint64_t NewCount = 0;

        [[nodiscard]] static std::optional<PickupNotification> Parse(std::span<const uint8_t> packet);
    };

    // The most units one withdrawal may ask for; the server refuses more.
    constexpr uint32_t MAX_WITHDRAW_UNITS = 100000;

    // "1,234,567".
    [[nodiscard]] std::wstring FormatCount(uint64_t value);

    // The amount typed into the amount field: digits only, at least 1. Anything else (empty, zero, letters, too large for
    // 32 bits) is null. The caller still limits it to the stored count.
    [[nodiscard]] std::optional<uint32_t> ParseAmount(const std::wstring& text);

    // The amount to withdraw: the typed amount limited to what the bank holds and to the per-request maximum;
    // zero when nothing can be withdrawn.
    [[nodiscard]] uint32_t WithdrawAmount(const std::optional<uint32_t>& typed, uint64_t stored);

    // "+3 Jewel of Bless -> Jewel Bank (1,204)"; empty for an unknown kind.
    [[nodiscard]] std::wstring PickupMessage(const PickupNotification& notification);
} // namespace JewelBank
