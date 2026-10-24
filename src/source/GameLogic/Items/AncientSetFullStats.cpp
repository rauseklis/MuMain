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
constexpr std::uint8_t StandardBonus = 100;
constexpr std::uint8_t BrassBonus = 75;
// Season 6 set-option identifiers: Strength, Agility, Vitality, Energy.
constexpr std::array<std::uint8_t, ExtraStatOptionCount> StatOptions = {0, 1, 3, 2};
} // namespace

void AddTierOneFullStatOptions(std::uint8_t setNumber, std::span<std::uint8_t> options, std::span<std::uint8_t> values)
{
    if (setNumber != HyperionSet && setNumber != BarnakeSet && setNumber != FaseSet && setNumber != CloudSet)
        return;
    if (options.size() != values.size())
        return;

    const std::uint8_t amount = setNumber == CloudSet ? BrassBonus : StandardBonus;
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
