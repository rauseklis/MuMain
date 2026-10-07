// <copyright file="AuctionWireResponses.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "AuctionWireResponses.h"

#include <cstring>

namespace AuctionHouse
{
    namespace
    {
        uint16_t ReadU16(std::span<const uint8_t> packet, size_t offset)
        {
            uint16_t value = 0;
            std::memcpy(&value, packet.data() + offset, sizeof(value));
            return value;
        }

        uint32_t ReadU32(std::span<const uint8_t> packet, size_t offset)
        {
            uint32_t value = 0;
            std::memcpy(&value, packet.data() + offset, sizeof(value));
            return value;
        }

        uint64_t ReadU64(std::span<const uint8_t> packet, size_t offset)
        {
            uint64_t value = 0;
            std::memcpy(&value, packet.data() + offset, sizeof(value));
            return value;
        }

        // Reads a six-field amount (scalar, strength, agility, vitality, energy, command) starting at offset, as the
        // server always writes all six regardless of currency; only the fields the currency mode actually uses are
        // meaningful.
        AuctionAmount ReadAmount(std::span<const uint8_t> packet, size_t offset, AuctionCurrencyMode currency)
        {
            const auto scalar = ReadU32(packet, offset);
            if (!IsFruitCurrency(currency))
            {
                return AuctionAmount::FromScalar(scalar);
            }

            return AuctionAmount::FromFruits(FruitBasket{
                static_cast<int64_t>(ReadU32(packet, offset + 4)),
                static_cast<int64_t>(ReadU32(packet, offset + 8)),
                static_cast<int64_t>(ReadU32(packet, offset + 12)),
                static_cast<int64_t>(ReadU32(packet, offset + 16)),
                static_cast<int64_t>(ReadU32(packet, offset + 20)),
            });
        }
    }

    std::optional<AuctionOperationResponse> AuctionOperationResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        AuctionOperationResponse response;
        std::memcpy(response.OperationId.data(), packet.data() + 4, response.OperationId.size());
        response.OperationType = static_cast<AuctionOperationType>(packet[20]);
        response.Result = static_cast<AuctionResult>(packet[21]);
        response.ListingId = ReadU64(packet, 22);
        response.CollectionId = ReadU64(packet, 30);
        response.ListingVersion = ReadU32(packet, 38);
        response.CurrencyMode = static_cast<AuctionCurrencyMode>(packet[42]);
        response.Fee = ReadAmount(packet, 43, response.CurrencyMode);
        response.Spendable = ReadAmount(packet, 67, response.CurrencyMode);
        response.Claimed = ReadAmount(packet, 91, response.CurrencyMode);
        response.Remaining = ReadAmount(packet, 115, response.CurrencyMode);
        response.ServerTime = ReadU32(packet, 139);
        return response;
    }

    std::optional<AuctionNotification> AuctionNotification::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        AuctionNotification notification;
        notification.NotificationKind = static_cast<AuctionNotificationKind>(packet[4]);
        notification.ListingId = ReadU64(packet, 5);
        notification.CurrencyMode = static_cast<AuctionCurrencyMode>(packet[13]);
        notification.Amount = ReadAmount(packet, 14, notification.CurrencyMode);
        notification.PendingMailboxCount = ReadU16(packet, 38);
        return notification;
    }
}
