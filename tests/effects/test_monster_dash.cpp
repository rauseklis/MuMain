#include "doctest.h"

#include "GameLogic/NPCs/MonsterDash.h"

using namespace GameLogic::MonsterDash;

TEST_CASE("a single tile keeps the minimum dash time")
{
    CHECK(DurationTicks(1.f) == doctest::Approx(MinimumTicks));
    CHECK(DurationTicks(0.f) == doctest::Approx(MinimumTicks));
}

TEST_CASE("a long path takes as long as the server's walk along it")
{
    CHECK(DurationTicks(6.f) == doctest::Approx(60.f));
    CHECK(DurationTicks(6.f) / 25.f == doctest::Approx(6 * 0.4f));
}

TEST_CASE("the dash time is capped")
{
    CHECK(DurationTicks(100.f) == doctest::Approx(MaximumTicks));
}

TEST_CASE("the dash starts at nothing and ends exactly at the target")
{
    CHECK(Progress(0.f, 20.f) == doctest::Approx(0.f));
    CHECK(Progress(20.f, 20.f) == doctest::Approx(1.f));
    CHECK(Progress(35.f, 20.f) == doctest::Approx(1.f));
}

TEST_CASE("the dash never moves backwards and is quicker at the start")
{
    float previous = 0.f;
    for (float tick = 0.f; tick <= 20.f; tick += 1.f)
    {
        const float progress = Progress(tick, 20.f);
        CHECK(progress >= previous);
        previous = progress;
    }
    CHECK(Progress(10.f, 20.f) > 0.5f);
}

TEST_CASE("the tile distance follows the longer axis")
{
    CHECK(TileDistance(0.f, 0.f, 3.f, -5.f) == doctest::Approx(5.f));
}
