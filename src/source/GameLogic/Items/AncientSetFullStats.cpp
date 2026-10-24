#include "AncientSetFullStats.h"

#include <algorithm>
#include <array>

namespace GameLogic::Items::Sets
{
namespace
{
constexpr std::uint8_t EmptyOption = 255;
constexpr std::uint8_t HyperionSet = 3;
constexpr std::uint8_t CloudSet = 8;
constexpr std::uint8_t BarnakeSet = 14;
constexpr std::uint8_t FaseSet = 24;
constexpr std::array<std::uint8_t, 6> TierTwoSets = {5,  7,  17,
                                                     27, 36, 38}; // Eplete, Garuda, Heras, Argo, Semeden, Chamer.
constexpr std::uint8_t TierTwoBonus = 50;
constexpr std::uint8_t StandardBonus = 100;
constexpr std::uint8_t BrassBonus = 75;
// Season 6 set-option identifiers: Strength, Agility, Vitality, Energy.
constexpr std::array<std::uint8_t, ExtraStatOptionCount> StatOptions = {0, 1, 3, 2};
std::uint8_t FullSetBonus(std::uint8_t setNumber)
{
    if (std::find(TierTwoSets.begin(), TierTwoSets.end(), setNumber) != TierTwoSets.end())
        return TierTwoBonus;
    if (setNumber == CloudSet)
        return BrassBonus;
    if (setNumber == HyperionSet || setNumber == BarnakeSet || setNumber == FaseSet)
        return StandardBonus;
    return 0;
}
} // namespace

void AddAncientFullStatOptions(std::uint8_t setNumber, std::span<std::uint8_t> options, std::span<std::uint8_t> values)
{
    const auto amount = FullSetBonus(setNumber);
    if (amount == 0)
        return;
    if (options.size() != values.size())
        return;

    for (const auto stat : StatOptions)
    {
        bool present = false;
        for (std::size_t i = 0; i < options.size(); ++i)
            present |= options[i] == stat && values[i] == amount;
        if (present)
            continue;

        const auto free = std::find(options.begin(), options.end(), EmptyOption);
        if (free == options.end())
            return;
        const auto index = std::distance(options.begin(), free);
        options[index] = stat;
        values[index] = amount;
    }
}
} // namespace GameLogic::Items::Sets
