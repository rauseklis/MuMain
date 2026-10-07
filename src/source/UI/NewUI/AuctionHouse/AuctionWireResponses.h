// <copyright file="AuctionWireResponses.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "AuctionModel.h"
#include "Dotnet/PacketFunctions_CommonEnums.h"

namespace AuctionHouse
{
    // The parsed result of one mutation (create/bid/buyout/cancel/collect), mirroring the server's
    // AuctionOperationResponse (wire 0xD5 sub 0x83, a C1-with-subcode packet: 4-byte header then this body).
    // Byte layout (offsets from the start of the body, i.e. from byte 4 of the full packet):
    //   OperationId[16]@0, OperationType(u8)@16, Result(u8)@17, ListingId(u64)@18, CollectionId(u64)@26,
    //   ListingVersion(u32)@34, CurrencyMode(u8)@38, Fee{Scalar,Str,Agi,Vit,Ene,Cmd}(u32x6)@39,
    //   Spendable{...}(u32x6)@63, Claimed{...}(u32x6)@87, Remaining{...}(u32x6)@111, ServerTime(u32)@135.
    // Total body length 139, full packet length 143.
    struct AuctionOperationResponse
    {
        static constexpr size_t FullWireLength = 143;
        static constexpr size_t HeaderLength = 4;

        std::array<uint8_t, 16> OperationId{};
        AuctionOperationType OperationType = AuctionOperationType::Create;
        AuctionResult Result = AuctionResult::Success;
        uint64_t ListingId = 0;
        uint64_t CollectionId = 0;
        uint32_t ListingVersion = 0;
        AuctionCurrencyMode CurrencyMode = AuctionCurrencyMode::Zen;
        AuctionAmount Fee = AuctionAmount::FromScalar(0);
        AuctionAmount Spendable = AuctionAmount::FromScalar(0);
        AuctionAmount Claimed = AuctionAmount::FromScalar(0);
        AuctionAmount Remaining = AuctionAmount::FromScalar(0);
        uint32_t ServerTime = 0;

        // Parses the full packet, header included (i.e. starting at the 0xC1 byte). Returns std::nullopt when the buffer
        // is shorter than the fixed wire length, so a malformed or truncated packet is never read out of bounds.
        [[nodiscard]] static std::optional<AuctionOperationResponse> Parse(std::span<const uint8_t> packet);
    };

    // The parsed result of a real-time Auction House notification, mirroring the server's AuctionNotification
    // (wire 0xD5 sub 0x86, a C1-with-subcode packet). Byte layout from the body (byte 4 of the full packet):
    //   NotificationKind(u8)@0, ListingId(u64)@1, CurrencyMode(u8)@9, Amount{Scalar,Str,Agi,Vit,Ene,Cmd}(u32x6)@10,
    //   PendingMailboxCount(u16)@34. Total body length 36, full packet length 40.
    struct AuctionNotification
    {
        static constexpr size_t FullWireLength = 40;
        static constexpr size_t HeaderLength = 4;

        AuctionNotificationKind NotificationKind = AuctionNotificationKind::Outbid;
        uint64_t ListingId = 0;
        AuctionCurrencyMode CurrencyMode = AuctionCurrencyMode::Zen;
        AuctionAmount Amount = AuctionAmount::FromScalar(0);
        uint16_t PendingMailboxCount = 0;

        [[nodiscard]] static std::optional<AuctionNotification> Parse(std::span<const uint8_t> packet);
    };
}
