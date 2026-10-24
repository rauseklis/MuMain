#include <doctest.h>

#include "GameLogic/Items/AncientSetFullStats.h"

#include <algorithm>
#include <array>

namespace
{
constexpr std::uint8_t EmptyOption = 255;
constexpr std::size_t OptionCount =
    GameLogic::Items::Sets::SerializedFullOptionCount + GameLogic::Items::Sets::ExtraStatOptionCount;
} // namespace

TEST_CASE("Tier 1 full-set stats preserve original full options and use approved amounts")
{
    for (const std::uint8_t set : {3, 14, 24, 8})
    {
        CAPTURE(set);
        std::array<std::uint8_t, OptionCount> options;
        std::array<std::uint8_t, OptionCount> values;
        options.fill(EmptyOption);
        values.fill(EmptyOption);
        options[0] = 19; // Existing skill damage option.
        values[0] = 20;
        options[1] = 12; // Existing maximum mana option.
        values[1] = 30;
        GameLogic::Items::Sets::AddTierOneFullStatOptions(set, options, values);
        CHECK(options[0] == 19);
        CHECK(values[0] == 20);
        CHECK(options[1] == 12);
        CHECK(values[1] == 30);
        const auto amount = set == 8 ? 75 : 100;
        const std::array<std::uint8_t, 4> stats = {0, 1, 3, 2}; // STR, AGI, VIT, ENE.
        for (const auto stat : stats)
        {
            const auto found = std::find(options.begin() + 2, options.end(), stat);
            REQUIRE(found != options.end());
            CHECK(values[std::distance(options.begin(), found)] == amount);
        }
        const auto expected = options;
        GameLogic::Items::Sets::AddTierOneFullStatOptions(set, options, values);
        CHECK(options == expected);
    }
}

TEST_CASE("Other Ancient tiers and serialized BMD record size are unchanged")
{
    static_assert(GameLogic::Items::Sets::SerializedFullOptionCount == 5);
    std::array<std::uint8_t, OptionCount> options;
    std::array<std::uint8_t, OptionCount> values;
    options.fill(EmptyOption);
    values.fill(EmptyOption);
    GameLogic::Items::Sets::AddTierOneFullStatOptions(11, options, values); // Hyon.
    CHECK(std::all_of(options.begin(), options.end(), [](auto option) { return option == EmptyOption; }));
}
