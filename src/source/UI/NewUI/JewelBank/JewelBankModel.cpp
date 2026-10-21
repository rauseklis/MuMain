#include "UI/NewUI/JewelBank/JewelBankModel.h"

#include <algorithm>

namespace JewelBank
{
    namespace
    {
        constexpr KindInfo KINDS[KIND_COUNT] = {
            { 1, L"Jewel of Chaos", 0xFFE8B84Au },
            { 2, L"Jewel of Bless", 0xFF7FD6FFu },
            { 3, L"Jewel of Soul", 0xFFFFA64Du },
            { 4, L"Jewel of Life", 0xFFFF7A7Au },
            { 5, L"Jewel of Creation", 0xFFB9F27Cu },
            { 6, L"Jewel of Guardian", 0xFFC9C9D6u },
            { 7, L"Jewel of Harmony", 0xFFE79BFFu },
            { 8, L"Gemstone", 0xFF8CE0B0u },
            { 9, L"Lower Refining Stone", 0xFFA6A6A6u },
            { 10, L"Higher Refining Stone", 0xFFD9D9D9u },
            { 11, L"Fruit of Energy", 0xFF5AA0FFu },
            { 12, L"Fruit of Vitality", 0xFFFF6B6Bu },
            { 13, L"Fruit of Agility", 0xFF6BE08Cu },
            { 14, L"Fruit of Strength", 0xFFFFC04Du },
            { 15, L"Fruit of Command", 0xFFC58BFFu },
        };

        uint32_t ReadU32(std::span<const uint8_t> data, size_t offset)
        {
            return static_cast<uint32_t>(data[offset]) | (static_cast<uint32_t>(data[offset + 1]) << 8)
                | (static_cast<uint32_t>(data[offset + 2]) << 16) | (static_cast<uint32_t>(data[offset + 3]) << 24);
        }

        uint64_t ReadU64(std::span<const uint8_t> data, size_t offset)
        {
            return static_cast<uint64_t>(ReadU32(data, offset)) | (static_cast<uint64_t>(ReadU32(data, offset + 4)) << 32);
        }

        // A result byte outside the known codes is treated as an invalid request, not as a success.
        Result ToResult(uint8_t value)
        {
            return value <= static_cast<uint8_t>(Result::ServiceUnavailable) ? static_cast<Result>(value) : Result::InvalidRequest;
        }
    } // namespace

    const KindInfo& KindAt(int index)
    {
        return KINDS[std::clamp(index, 0, KIND_COUNT - 1)];
    }

    const KindInfo* FindKind(uint8_t code)
    {
        if (code < FIRST_KIND_CODE || code >= FIRST_KIND_CODE + KIND_COUNT)
        {
            return nullptr;
        }
        return &KINDS[code - FIRST_KIND_CODE];
    }

    const wchar_t* ResultText(Result result)
    {
        switch (result)
        {
        case Result::Success: return L"Done.";
        case Result::InvalidRequest: return L"That request is not valid.";
        case Result::InsufficientBalance: return L"The bank does not hold that many.";
        case Result::InventoryFull: return L"Not enough free space in your inventory.";
        case Result::CapReached: return L"The bank is full for that item.";
        case Result::NothingToDeposit: return L"Nothing to deposit. Bundles are deposited whole.";
        case Result::NotConfigured: return L"This item is not available on the server.";
        case Result::StateRestricted: return L"Go to a safe zone and close other windows first.";
        case Result::RateLimited: return L"Please wait a moment.";
        case Result::ServiceUnavailable: return L"The Jewel Bank is not available.";
        }
        return L"";
    }

    std::optional<StateResponse> StateResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        StateResponse response;
        response.Result = ToResult(packet[4]);
        response.AutoDeposit = packet[5] != 0;
        for (size_t i = 0; i < response.Counts.size(); ++i)
        {
            response.Counts[i] = ReadU64(packet, 6 + 8 * i);
            response.Inventory[i] = ReadU32(packet, 126 + 4 * i);
        }
        return response;
    }

    std::optional<OperationResponse> OperationResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        OperationResponse response;
        std::copy_n(packet.begin() + 4, response.OperationId.size(), response.OperationId.begin());
        response.Operation = static_cast<JewelBank::Operation>(packet[20]);
        response.Result = ToResult(packet[21]);
        response.KindCode = packet[22];
        response.Units = ReadU32(packet, 23);
        return response;
    }

    std::optional<PickupNotification> PickupNotification::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        PickupNotification notification;
        notification.KindCode = packet[4];
        notification.Units = ReadU32(packet, 5);
        notification.NewCount = ReadU64(packet, 9);
        return notification;
    }

    std::wstring FormatCount(uint64_t value)
    {
        std::wstring digits = std::to_wstring(value);
        std::wstring result;
        const size_t length = digits.size();
        for (size_t i = 0; i < length; ++i)
        {
            if (i != 0 && (length - i) % 3 == 0)
            {
                result += L',';
            }
            result += digits[i];
        }
        return result;
    }

    std::optional<uint32_t> ParseAmount(const std::wstring& text)
    {
        if (text.empty() || text.size() > 10)
        {
            return std::nullopt;
        }

        uint64_t value = 0;
        for (const wchar_t c : text)
        {
            if (c < L'0' || c > L'9')
            {
                return std::nullopt;
            }
            value = value * 10 + static_cast<uint64_t>(c - L'0');
        }

        if (value == 0 || value > UINT32_MAX)
        {
            return std::nullopt;
        }
        return static_cast<uint32_t>(value);
    }

    uint32_t ClampAmount(const std::optional<uint32_t>& typed, uint64_t available)
    {
        if (!typed.has_value() || available == 0)
        {
            return 0;
        }

        const uint64_t limited = std::min<uint64_t>({ *typed, available, MAX_WITHDRAW_UNITS });
        return static_cast<uint32_t>(limited);
    }

    uint32_t ToggleAmount(uint32_t current, uint64_t available)
    {
        if (current > 0)
        {
            return 0;
        }
        return ClampAmount(UINT32_MAX, available);
    }

    Amounts SelectAll(const Available& available)
    {
        Amounts amounts{};
        for (size_t i = 0; i < amounts.size(); ++i)
        {
            amounts[i] = ClampAmount(UINT32_MAX, available[i]);
        }
        return amounts;
    }

    void LimitToAvailable(Amounts& amounts, const Available& available)
    {
        for (size_t i = 0; i < amounts.size(); ++i)
        {
            amounts[i] = std::min<uint64_t>(amounts[i], std::min<uint64_t>(available[i], MAX_WITHDRAW_UNITS));
        }
    }

    int SelectedKinds(const Amounts& amounts)
    {
        return static_cast<int>(std::count_if(amounts.begin(), amounts.end(), [](uint32_t amount) { return amount > 0; }));
    }

    uint64_t TotalAmount(const Amounts& amounts)
    {
        uint64_t total = 0;
        for (const uint32_t amount : amounts)
        {
            total += amount;
        }
        return total;
    }

    std::wstring PickupMessage(const PickupNotification& notification)
    {
        const KindInfo* kind = FindKind(notification.KindCode);
        if (kind == nullptr)
        {
            return {};
        }

        return L"+" + std::to_wstring(notification.Units) + L" " + kind->Name + L" -> Jewel Bank (" + FormatCount(notification.NewCount) + L")";
    }
} // namespace JewelBank
