#include "doctest.h"

#include "MUHelper/HuntingArea.h"

namespace Area = MUHelper::HuntingArea;
using Area::Tile;

TEST_CASE("distance is the straight line in tiles, rounded up")
{
    CHECK(Area::Distance({0, 0}, {3, 4}) == 5);
    CHECK(Area::Distance({10, 10}, {10, 10}) == 0);
    CHECK(Area::Distance({0, 0}, {6, 6}) == 9);
}

TEST_CASE("the hunting area is measured from where the helper was started")
{
    const Tile start{100, 100};
    const int huntingDistance = 9;

    CHECK(Area::Contains(start, huntingDistance, {109, 100}, {109, 100}));
    CHECK_FALSE(Area::Contains(start, huntingDistance, {110, 100}, {110, 100}));
    // A monster 20 tiles from the start is outside, wherever the character stands now.
    CHECK_FALSE(Area::Contains(start, huntingDistance, {120, 100}, {120, 100}));
}

TEST_CASE("a monster walking into the hunting area counts as inside")
{
    CHECK(Area::Contains({100, 100}, 9, {112, 100}, {105, 100}));
}

TEST_CASE("self defense ignores a monster that only moved or appeared")
{
    CHECK_FALSE(Area::ShouldDefendAgainst(true, false, true, false));
    CHECK_FALSE(Area::ShouldDefendAgainst(true, false, false, true));
}

TEST_CASE("self defense answers an attacker inside the hunting area")
{
    CHECK(Area::ShouldDefendAgainst(true, true, true, false));
}

TEST_CASE("self defense answers a far attacker only with long-range counter attack")
{
    CHECK_FALSE(Area::ShouldDefendAgainst(true, true, false, false));
    CHECK(Area::ShouldDefendAgainst(true, true, false, true));
}

TEST_CASE("without self defense nothing switches the target")
{
    CHECK_FALSE(Area::ShouldDefendAgainst(false, true, true, true));
}

namespace
{
struct RangeScenario
{
    const char* name;
    Tile monster;
    Tile destination;
    bool expectedInside;
};

// Inside the circle exactly when dx * dx + dy * dy <= distance * distance: worked out without a square root, as a
// second opinion on Contains().
bool InsideBySquares(Tile start, int huntingDistance, Tile tile)
{
    const int dx = tile.x - start.x;
    const int dy = tile.y - start.y;
    return dx * dx + dy * dy <= huntingDistance * huntingDistance;
}
} // namespace

TEST_CASE("100 hunting range scenarios: ranges 1 to 10, ten monster positions each")
{
    const Tile start{100, 100};
    int scenarios = 0;

    for (int range = 1; range <= 10; ++range)
    {
        // The helper turns its range setting into the distance to the corner of a range-by-range square.
        const int d = Area::Distance({0, 0}, {range, range});
        const int far = 25;

        const RangeScenario cases[] = {
            {"on the edge, east", {start.x + d, start.y}, {start.x + d, start.y}, true},
            {"one past the edge, east", {start.x + d + 1, start.y}, {start.x + d + 1, start.y}, false},
            {"on the edge, north", {start.x, start.y - d}, {start.x, start.y - d}, true},
            {"one past the edge, south", {start.x, start.y + d + 1}, {start.x, start.y + d + 1}, false},
            {"on the edge, west", {start.x - d, start.y}, {start.x - d, start.y}, true},
            {"range corner, diagonal", {start.x + range, start.y + range}, {start.x + range, start.y + range}, true},
            {"one past the corner, diagonal",
             {start.x + range + 1, start.y + range + 1},
             {start.x + range + 1, start.y + range + 1},
             false},
            {"outside, walking in", {start.x + d + 3, start.y}, {start.x + d - 1, start.y}, true},
            {"inside, walking out", {start.x - d + 1, start.y}, {start.x - d - 3, start.y}, true},
            {"25 tiles away", {start.x + far, start.y + 1}, {start.x + far, start.y + 1}, d >= 26},
        };

        for (const RangeScenario& scenario : cases)
        {
            CAPTURE(range);
            CAPTURE(d);
            CAPTURE(scenario.name);

            const bool inside = Area::Contains(start, d, scenario.monster, scenario.destination);
            CHECK(inside == scenario.expectedInside);

            const bool bySquares =
                InsideBySquares(start, d, scenario.monster) || InsideBySquares(start, d, scenario.destination);
            CHECK(inside == bySquares);
            ++scenarios;
        }
    }

    CHECK(scenarios == 100);
}

TEST_CASE("self defense: every combination of its four inputs")
{
    for (int mask = 0; mask < 16; ++mask)
    {
        const bool selfDefense = (mask & 1) != 0;
        const bool isAttacking = (mask & 2) != 0;
        const bool insideArea = (mask & 4) != 0;
        const bool longRange = (mask & 8) != 0;
        CAPTURE(mask);

        const bool expected = selfDefense && isAttacking && (insideArea || longRange);
        CHECK(Area::ShouldDefendAgainst(selfDefense, isAttacking, insideArea, longRange) == expected);
    }
}
