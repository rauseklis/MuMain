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

    // The full detail of one listing, mirroring the server's AuctionDetailResponse (0xD5 sub 0x82, a
    // C2-with-subcode packet). Fixed 204 bytes. Byte layout (absolute offsets): RequestId(u32)@5,
    // Result(u8)@9, ServerTime(u32)@10, ListingId(u64)@14, Version(u32)@22, Status(u8)@26,
    // CurrencyMode(u8)@27, Category(u8)@28, CurrentPrice{6}(u32x6)@29, BuyoutPrice{6}(u32x6)@53,
    // BidCount(u16)@77, OriginalEndsAt(u32)@79, EndsAt(u32)@83, SellerName(10 bytes)@87,
    // CurrentBidderName(10 bytes)@97, NoteLength(u8)@107, Note(80 bytes)@108, ItemDataLength(u8)@188,
    // ItemData(15 bytes)@189.
    struct AuctionDetailResponse
    {
        static constexpr size_t FullWireLength = 204;

        uint32_t RequestId = 0;
        AuctionResult Result = AuctionResult::Success;
        uint32_t ServerTime = 0;
        uint64_t ListingId = 0;
        uint32_t Version = 0;
        AuctionListingStatus Status = AuctionListingStatus::Active;
        AuctionCurrencyMode CurrencyMode = AuctionCurrencyMode::Zen;
        AuctionCategory Category = AuctionCategory::Miscellaneous;
        AuctionAmount CurrentPrice = AuctionAmount::FromScalar(0);
        AuctionAmount BuyoutPrice = AuctionAmount::FromScalar(0);
        uint16_t BidCount = 0;
        uint32_t OriginalEndsAt = 0;
        uint32_t EndsAt = 0;
        std::wstring SellerName;
        std::wstring CurrentBidderName;
        uint8_t NoteLength = 0;
        // Widened byte-for-byte (Latin-1), not UTF-8 decoded: no UI consumes this field yet (the Sell/Detail
        // tabs are tasks 4.3/4.4), and that UI will run inside the engine's own translation unit, where the
        // existing CMultiLanguage::ConvertFromUtf8 (MultiLanguage.cpp) is reachable. Revisit then if notes
        // should round-trip non-ASCII text.
        std::wstring Note;
        uint8_t ItemDataLength = 0;
        std::array<uint8_t, 15> ItemData{};

        [[nodiscard]] static std::optional<AuctionDetailResponse> Parse(std::span<const uint8_t> packet);
    };

    // One pending Mailbox row, mirroring the server's AuctionMailboxEntryRef. Always exactly 88 bytes.
    // Byte layout: CollectionId(u64)@0, CollectionKind(u8)@8, CollectionStatus(u8)@9, CurrencyMode(u8)@10,
    // Original{Scalar,Str,Agi,Vit,Ene,Cmd}(u32x6)@11 (the full amount this collection was created with),
    // Remaining{...same 6}(u32x6)@35 (what is left to claim; equal to Original until partially claimed),
    // SourceListingId(u64)@59, HasItem(bool)@67, ItemDataLength(u8)@68, ItemData(15 bytes)@69, Version(u32)@84.
    struct AuctionMailboxEntry
    {
        static constexpr size_t WireLength = 88;

        uint64_t CollectionId = 0;
        AuctionCollectionKind CollectionKind = AuctionCollectionKind::PurchasedItem;
        AuctionCollectionStatus CollectionStatus = AuctionCollectionStatus::Pending;
        AuctionCurrencyMode CurrencyMode = AuctionCurrencyMode::Zen;
        AuctionAmount Original = AuctionAmount::FromScalar(0);
        AuctionAmount Remaining = AuctionAmount::FromScalar(0);
        uint64_t SourceListingId = 0;
        bool HasItem = false;
        uint8_t ItemDataLength = 0;
        std::array<uint8_t, 15> ItemData{};
        uint32_t Version = 0;

        [[nodiscard]] static std::optional<AuctionMailboxEntry> Parse(std::span<const uint8_t> entry);
    };

    // A page of pending Mailbox entries, mirroring the server's AuctionMailboxResponse (0xD5 sub 0x85, a
    // C2-with-subcode packet). Byte layout (absolute offsets): RequestId(u32)@5, Result(u8)@9, Page(u16)@10,
    // TotalPages(u16)@12, TotalCount(u32)@14, ServerTime(u32)@18, EntryCount(u8)@22, then EntryCount
    // AuctionMailboxEntry entries starting at 23.
    struct AuctionMailboxResponse
    {
        static constexpr size_t FixedWireLength = 23;

        uint32_t RequestId = 0;
        AuctionResult Result = AuctionResult::Success;
        uint16_t Page = 0;
        uint16_t TotalPages = 0;
        uint32_t TotalCount = 0;
        uint32_t ServerTime = 0;
        std::vector<AuctionMailboxEntry> Entries;

        [[nodiscard]] static std::optional<AuctionMailboxResponse> Parse(std::span<const uint8_t> packet);
    };

    // One currency's rules and the player's spendable units in it, mirroring the server's
    // AuctionCurrencyDescriptorRef. Version 4 is 72 bytes (version 3 was 57, version 2 was 45, version 1 was 39). Byte layout: CurrencyMode(u8)@0,
    // Enabled(bool)@1, MinimumStartingPrice(u32)@2, MaximumStartingPrice(u32)@6, MinimumIncrement(u32)@10,
    // IncrementPercent(u8)@14, Spendable{Scalar,Str,Agi,Vit,Ene,Cmd}(u32x6)@15, then the server-authoritative
    // small/medium/large packed-jewel denominations (u16x3)@39, loose/pack item types (u16x2)@45,
    // and loose/small/medium/large inventory counts (u16x4)@49. Version 4 adds the server-authored
    // Type/Level identity of each stat Fruit (u16x5 then u8x5)@57. Older descriptors default missing metadata to zero.
    struct AuctionCurrencyDescriptor
    {
        static constexpr size_t LegacyWireLength = 39;
        static constexpr size_t PackMetadataWireLength = 45;
        static constexpr size_t InventoryMetadataWireLength = 57;
        static constexpr size_t WireLength = 72;

        AuctionCurrencyMode CurrencyMode = AuctionCurrencyMode::Zen;
        bool Enabled = false;
        uint32_t MinimumStartingPrice = 0;
        uint32_t MaximumStartingPrice = 0;
        uint32_t MinimumIncrement = 0;
        uint8_t IncrementPercent = 0;
        AuctionAmount Spendable = AuctionAmount::FromScalar(0);
        std::array<uint16_t, 3> PackUnits{};
        uint16_t LooseItemType = 0;
        uint16_t PackItemType = 0;
        uint16_t LooseItemCount = 0;
        std::array<uint16_t, 3> PackCounts{};
        std::array<uint16_t, 5> FruitItemTypes{};
        std::array<uint8_t, 5> FruitItemLevels{};

        [[nodiscard]] static std::optional<AuctionCurrencyDescriptor> Parse(std::span<const uint8_t> entry);
    };

    // The response to opening the Auction House window, mirroring the server's AuctionOpenResponse (0xD5 sub
    // 0x80, a C2-with-subcode packet). Byte layout (absolute offsets): RequestId(u32)@5, Result(u8)@9,
    // ServerTime(u32)@10, ConfigurationVersion(u32)@14, ListingFeeBasisPoints(u16)@18,
    // SuccessFeeBasisPoints(u16)@20, DurationMask(u8)@22, DefaultDurationHours(u8)@23, PageSize(u8)@24,
    // PendingMailboxCount(u16)@25, CurrencyCount(u8)@27, then CurrencyCount AuctionCurrencyDescriptor
    // entries starting at 28.
    struct AuctionOpenResponse
    {
        static constexpr size_t FixedWireLength = 28;

        uint32_t RequestId = 0;
        AuctionResult Result = AuctionResult::Success;
        uint32_t ServerTime = 0;
        uint32_t ConfigurationVersion = 0;
        uint16_t ListingFeeBasisPoints = 0;
        uint16_t SuccessFeeBasisPoints = 0;
        uint8_t DurationMask = 0;
        uint8_t DefaultDurationHours = 0;
        uint8_t PageSize = 0;
        uint16_t PendingMailboxCount = 0;
        std::vector<AuctionCurrencyDescriptor> Currencies;

        [[nodiscard]] static std::optional<AuctionOpenResponse> Parse(std::span<const uint8_t> packet);
    };
}
