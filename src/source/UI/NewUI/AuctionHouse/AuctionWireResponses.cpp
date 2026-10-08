// <copyright file="AuctionWireResponses.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "AuctionWireResponses.h"

#include <algorithm>
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

        // Reads up to length raw ASCII bytes as a wide string, stopping at the first NUL (the field may be
        // shorter than its fixed width and zero-padded, same convention as the legacy character name fields).
        std::wstring ReadFixedAscii(std::span<const uint8_t> packet, size_t offset, size_t length)
        {
            std::wstring text;
            text.reserve(length);
            for (size_t i = 0; i < length; ++i)
            {
                const auto byte = packet[offset + i];
                if (byte == 0)
                {
                    break;
                }

                text.push_back(static_cast<wchar_t>(byte));
            }

            return text;
        }
    }

    namespace
    {
        constexpr size_t SellerNameLength = 10;

        std::optional<AuctionListingSummary> ParseListingSummaryAt(std::span<const uint8_t> packet, size_t offset)
        {
            if (packet.size() < offset + AuctionListingSummary::WireLength)
            {
                return std::nullopt;
            }

            AuctionListingSummary summary;
            summary.ListingId = ReadU64(packet, offset + 0);
            summary.Version = ReadU32(packet, offset + 8);
            summary.Status = static_cast<AuctionListingStatus>(packet[offset + 12]);
            summary.CurrencyMode = static_cast<AuctionCurrencyMode>(packet[offset + 13]);
            summary.Category = static_cast<AuctionCategory>(packet[offset + 14]);
            summary.CurrentPrice = ReadAmount(packet, offset + 15, summary.CurrencyMode);
            summary.BuyoutPrice = ReadAmount(packet, offset + 39, summary.CurrencyMode);
            summary.BidCount = ReadU16(packet, offset + 63);
            summary.EndsAt = ReadU32(packet, offset + 65);
            summary.SellerName = ReadFixedAscii(packet, offset + 69, SellerNameLength);
            summary.ItemDataLength = packet[offset + 79];
            std::memcpy(summary.ItemData.data(), packet.data() + offset + 80, summary.ItemData.size());
            return summary;
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

    std::optional<AuctionListingSummary> AuctionListingSummary::Parse(std::span<const uint8_t> entry)
    {
        return ParseListingSummaryAt(entry, 0);
    }

    std::optional<AuctionBrowseResponse> AuctionBrowseResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FixedWireLength)
        {
            return std::nullopt;
        }

        AuctionBrowseResponse response;
        response.RequestId = ReadU32(packet, 5);
        response.Result = static_cast<AuctionResult>(packet[9]);
        response.Page = ReadU16(packet, 10);
        response.TotalPages = ReadU16(packet, 12);
        response.TotalCount = ReadU32(packet, 14);
        response.ServerTime = ReadU32(packet, 18);

        const auto listingCount = packet[22];
        response.Listings.reserve(listingCount);
        for (uint8_t i = 0; i < listingCount; ++i)
        {
            auto summary = ParseListingSummaryAt(packet, FixedWireLength + static_cast<size_t>(i) * AuctionListingSummary::WireLength);
            if (!summary)
            {
                return std::nullopt;
            }

            response.Listings.push_back(std::move(*summary));
        }

        return response;
    }

    std::optional<AuctionDetailResponse> AuctionDetailResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FullWireLength)
        {
            return std::nullopt;
        }

        AuctionDetailResponse response;
        response.RequestId = ReadU32(packet, 5);
        response.Result = static_cast<AuctionResult>(packet[9]);
        response.ServerTime = ReadU32(packet, 10);
        response.ListingId = ReadU64(packet, 14);
        response.Version = ReadU32(packet, 22);
        response.Status = static_cast<AuctionListingStatus>(packet[26]);
        response.CurrencyMode = static_cast<AuctionCurrencyMode>(packet[27]);
        response.Category = static_cast<AuctionCategory>(packet[28]);
        response.CurrentPrice = ReadAmount(packet, 29, response.CurrencyMode);
        response.BuyoutPrice = ReadAmount(packet, 53, response.CurrencyMode);
        response.BidCount = ReadU16(packet, 77);
        response.OriginalEndsAt = ReadU32(packet, 79);
        response.EndsAt = ReadU32(packet, 83);
        response.SellerName = ReadFixedAscii(packet, 87, SellerNameLength);
        response.CurrentBidderName = ReadFixedAscii(packet, 97, SellerNameLength);
        response.NoteLength = packet[107];
        // Clamp to the field's actual 80-byte width: NoteLength is a server-supplied byte and must not be
        // trusted to stay within bounds on its own.
        constexpr size_t NoteFieldWidth = 80;
        response.Note = ReadFixedAscii(packet, 108, std::min<size_t>(response.NoteLength, NoteFieldWidth));
        response.ItemDataLength = packet[188];
        std::memcpy(response.ItemData.data(), packet.data() + 189, response.ItemData.size());
        return response;
    }

    std::optional<AuctionMailboxEntry> AuctionMailboxEntry::Parse(std::span<const uint8_t> entry)
    {
        if (entry.size() < WireLength)
        {
            return std::nullopt;
        }

        AuctionMailboxEntry parsed;
        parsed.CollectionId = ReadU64(entry, 0);
        parsed.CollectionKind = static_cast<AuctionCollectionKind>(entry[8]);
        parsed.CollectionStatus = static_cast<AuctionCollectionStatus>(entry[9]);
        parsed.CurrencyMode = static_cast<AuctionCurrencyMode>(entry[10]);
        parsed.Original = ReadAmount(entry, 11, parsed.CurrencyMode);
        parsed.Remaining = ReadAmount(entry, 35, parsed.CurrencyMode);
        parsed.SourceListingId = ReadU64(entry, 59);
        parsed.HasItem = entry[67] != 0;
        parsed.ItemDataLength = entry[68];
        std::memcpy(parsed.ItemData.data(), entry.data() + 69, parsed.ItemData.size());
        parsed.Version = ReadU32(entry, 84);
        return parsed;
    }

    std::optional<AuctionMailboxResponse> AuctionMailboxResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FixedWireLength)
        {
            return std::nullopt;
        }

        AuctionMailboxResponse response;
        response.RequestId = ReadU32(packet, 5);
        response.Result = static_cast<AuctionResult>(packet[9]);
        response.Page = ReadU16(packet, 10);
        response.TotalPages = ReadU16(packet, 12);
        response.TotalCount = ReadU32(packet, 14);
        response.ServerTime = ReadU32(packet, 18);

        const auto entryCount = packet[22];
        response.Entries.reserve(entryCount);
        for (uint8_t i = 0; i < entryCount; ++i)
        {
            const auto offset = FixedWireLength + static_cast<size_t>(i) * AuctionMailboxEntry::WireLength;
            if (packet.size() < offset + AuctionMailboxEntry::WireLength)
            {
                return std::nullopt;
            }

            auto parsedEntry = AuctionMailboxEntry::Parse(packet.subspan(offset));
            if (!parsedEntry)
            {
                return std::nullopt;
            }

            response.Entries.push_back(std::move(*parsedEntry));
        }

        return response;
    }

    std::optional<AuctionCurrencyDescriptor> AuctionCurrencyDescriptor::Parse(std::span<const uint8_t> entry)
    {
        if (entry.size() < LegacyWireLength)
        {
            return std::nullopt;
        }

        AuctionCurrencyDescriptor descriptor;
        descriptor.CurrencyMode = static_cast<AuctionCurrencyMode>(entry[0]);
        descriptor.Enabled = entry[1] != 0;
        descriptor.MinimumStartingPrice = ReadU32(entry, 2);
        descriptor.MaximumStartingPrice = ReadU32(entry, 6);
        descriptor.MinimumIncrement = ReadU32(entry, 10);
        descriptor.IncrementPercent = entry[14];
        descriptor.Spendable = ReadAmount(entry, 15, descriptor.CurrencyMode);
        if (entry.size() >= PackMetadataWireLength)
        {
            descriptor.PackUnits = {ReadU16(entry, 39), ReadU16(entry, 41), ReadU16(entry, 43)};
        }
        if (entry.size() >= WireLength)
        {
            descriptor.LooseItemType = ReadU16(entry, 45);
            descriptor.PackItemType = ReadU16(entry, 47);
            descriptor.LooseItemCount = ReadU16(entry, 49);
            descriptor.PackCounts = {ReadU16(entry, 51), ReadU16(entry, 53), ReadU16(entry, 55)};
        }

        return descriptor;
    }

    std::optional<AuctionOpenResponse> AuctionOpenResponse::Parse(std::span<const uint8_t> packet)
    {
        if (packet.size() < FixedWireLength)
        {
            return std::nullopt;
        }

        AuctionOpenResponse response;
        response.RequestId = ReadU32(packet, 5);
        response.Result = static_cast<AuctionResult>(packet[9]);
        response.ServerTime = ReadU32(packet, 10);
        response.ConfigurationVersion = ReadU32(packet, 14);
        response.ListingFeeBasisPoints = ReadU16(packet, 18);
        response.SuccessFeeBasisPoints = ReadU16(packet, 20);
        response.DurationMask = packet[22];
        response.DefaultDurationHours = packet[23];
        response.PageSize = packet[24];
        response.PendingMailboxCount = ReadU16(packet, 25);

        const auto currencyCount = packet[27];
        const auto descriptorWireLength = response.ConfigurationVersion >= 3
            ? AuctionCurrencyDescriptor::WireLength
            : response.ConfigurationVersion >= 2
                ? AuctionCurrencyDescriptor::PackMetadataWireLength
                : AuctionCurrencyDescriptor::LegacyWireLength;
        response.Currencies.reserve(currencyCount);
        for (uint8_t i = 0; i < currencyCount; ++i)
        {
            const auto offset = FixedWireLength + static_cast<size_t>(i) * descriptorWireLength;
            if (packet.size() < offset + descriptorWireLength)
            {
                return std::nullopt;
            }

            auto descriptor = AuctionCurrencyDescriptor::Parse(packet.subspan(offset, descriptorWireLength));
            if (!descriptor)
            {
                return std::nullopt;
            }

            response.Currencies.push_back(std::move(*descriptor));
        }

        return response;
    }
}
