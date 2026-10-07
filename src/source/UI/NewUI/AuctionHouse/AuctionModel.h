// <copyright file="AuctionModel.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "Dotnet/PacketFunctions_CommonEnums.h"

namespace AuctionHouse
{
    // The five fruit components of a Fruits-currency listing or bid. Zero in every field means "no fruits set".
    struct FruitBasket
    {
        int64_t Strength = 0;
        int64_t Agility = 0;
        int64_t Vitality = 0;
        int64_t Energy = 0;
        int64_t Command = 0;

        [[nodiscard]] bool IsZero() const noexcept
        {
            return this->Strength == 0 && this->Agility == 0 && this->Vitality == 0 && this->Energy == 0 && this->Command == 0;
        }
    };

    [[nodiscard]] inline bool operator==(const FruitBasket& left, const FruitBasket& right) noexcept
    {
        return left.Strength == right.Strength && left.Agility == right.Agility && left.Vitality == right.Vitality
            && left.Energy == right.Energy && left.Command == right.Command;
    }

    [[nodiscard]] inline FruitBasket operator+(const FruitBasket& left, const FruitBasket& right) noexcept
    {
        return FruitBasket{
            left.Strength + right.Strength,
            left.Agility + right.Agility,
            left.Vitality + right.Vitality,
            left.Energy + right.Energy,
            left.Command + right.Command,
        };
    }

    // A listing/bid/buyout amount: a single scalar unit count for Zen or a jewel currency, or a fruit basket for the Fruits
    // currency. Prices never compare across currencies, so an amount is always read together with its listing's currency.
    class AuctionAmount
    {
    public:
        [[nodiscard]] static AuctionAmount FromScalar(int64_t units) noexcept
        {
            AuctionAmount amount;
            amount._isFruitBasket = false;
            amount._scalar = units;
            return amount;
        }

        [[nodiscard]] static AuctionAmount FromFruits(const FruitBasket& basket) noexcept
        {
            AuctionAmount amount;
            amount._isFruitBasket = true;
            amount._fruits = basket;
            return amount;
        }

        [[nodiscard]] bool IsFruitBasket() const noexcept { return this->_isFruitBasket; }

        [[nodiscard]] int64_t Scalar() const noexcept { return this->_scalar; }

        [[nodiscard]] const FruitBasket& Fruits() const noexcept { return this->_fruits; }

        [[nodiscard]] bool IsZero() const noexcept
        {
            return this->_isFruitBasket ? this->_fruits.IsZero() : this->_scalar == 0;
        }

    private:
        bool _isFruitBasket = false;
        int64_t _scalar = 0;
        FruitBasket _fruits{};
    };

    // Whether a currency mode settles in a fruit basket rather than a single scalar unit count.
    [[nodiscard]] inline bool IsFruitCurrency(AuctionCurrencyMode mode) noexcept
    {
        return mode == AuctionCurrencyMode::Fruits;
    }

    // Formats the time remaining on a listing for the countdown display: "Ended" once it has reached zero, otherwise the two
    // most significant units ("1d 1h", "1h 1m", "2m 5s", or "45s" under a minute).
    [[nodiscard]] std::wstring FormatAuctionCountdown(std::chrono::seconds remaining);
}
