// <copyright file="AuctionWireResponses.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

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

    // One browse/my-listings row, mirroring the server's AuctionListingSummary. Always exactly 95 bytes; a
    // browse or my-listings response is a sequence of these starting right after its own fixed header.
    // Byte layout: ListingId(u64)@0, Version(u32)@8, Status(u8)@12, CurrencyMode(u8)@13, Category(u8)@14,
    //   CurrentPrice{Scalar,Str,Agi,Vit,Ene,Cmd}(u32x6)@15 (the effective price: current bid if any, else
    //   starting price), BuyoutPrice{...same 6}(u32x6)@39 (all-zero means no buyout), BidCount(u16)@63,
    //   EndsAt(u32)@65, SellerName(10 raw bytes)@69, ItemDataLength(u8)@79, ItemData(15 raw bytes)@80.
    struct AuctionListingSummary
    {
        static constexpr size_t WireLength = 95;

        uint64_t ListingId = 0;
        uint32_t Version = 0;
        AuctionListingStatus Status = AuctionListingStatus::Active;
        AuctionCurrencyMode CurrencyMode = AuctionCurrencyMode::Zen;
        AuctionCategory Category = AuctionCategory::Miscellaneous;
        AuctionAmount CurrentPrice = AuctionAmount::FromScalar(0);
        AuctionAmount BuyoutPrice = AuctionAmount::FromScalar(0);
        uint16_t BidCount = 0;
        uint32_t EndsAt = 0;
        std::wstring SellerName;
        uint8_t ItemDataLength = 0;
        std::array<uint8_t, 15> ItemData{};

        // Parses exactly WireLength bytes from the start of entry. Returns std::nullopt when entry is shorter.
        [[nodiscard]] static std::optional<AuctionListingSummary> Parse(std::span<const uint8_t> entry);
    };

    // A page of browse results, mirroring the server's AuctionBrowseResponse (0xD5 sub 0x81, a C2-with-subcode
    // packet). Byte layout (absolute offsets in the full packet, same convention as AuctionOperationResponse
    // and AuctionNotification above): RequestId(u32)@5, Result(u8)@9, Page(u16)@10, TotalPages(u16)@12,
    // TotalCount(u32)@14, ServerTime(u32)@18, ListingCount(u8)@22, then ListingCount AuctionListingSummary
    // entries starting at 23.
    struct AuctionBrowseResponse
    {
        static constexpr size_t FixedWireLength = 23;

        uint32_t RequestId = 0;
        AuctionResult Result = AuctionResult::Success;
        uint16_t Page = 0;
        uint16_t TotalPages = 0;
        uint32_t TotalCount = 0;
        uint32_t ServerTime = 0;
        std::vector<AuctionListingSummary> Listings;

        // Returns std::nullopt when the buffer is shorter than its own fixed header, or shorter than the header
        // plus ListingCount entries (a truncated packet), so a malformed packet is never read out of bounds.
        [[nodiscard]] static std::optional<AuctionBrowseResponse> Parse(std::span<const uint8_t> packet);
    };

    // A page of the account's own listings, mirroring the server's AuctionMyListingsResponse (0xD5 sub 0x84).
    // Byte-for-byte identical shape to AuctionBrowseResponse (same fixed header, same AuctionListingSummary
    // entries) — only the sub-code and the server-side query differ, so this reuses the same parser.
    using AuctionMyListingsResponse = AuctionBrowseResponse;
}
