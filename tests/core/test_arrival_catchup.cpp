#include "doctest.h"

#include "Engine/AI/ArrivalCatchup.h"

#include <initializer_list>

using Engine::Movement::UpdateArrivalCatchup;

TEST_CASE("A new walk cancels the previous arrival correction [core][movement]")
{
    // Captured from the stuck character: the old target was x=4650 while
    // a two-point helper path was trying to walk toward x=4850 or x=4950.
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CAPTURE(fps);
        float position[] = {4657.0f, 18950.0f, 215.0f};
        bool active = true;
        const float frameFactor = 25.0f / fps;
        constexpr float WalkSpeed = 10.0f;
        constexpr float DurationSeconds = 2.0f;
        for (int frame = 0; frame < static_cast<int>(fps * DurationSeconds); ++frame)
        {
            position[0] += WalkSpeed * frameFactor;
            active = UpdateArrivalCatchup(position, 4650.0f, 18950.0f, active, true, frameFactor);
        }
        CHECK_FALSE(active);
        CHECK(position[0] > 4950.0f);
        CHECK(position[1] == 18950.0f);
        CHECK(position[2] == 215.0f);
    }
}

TEST_CASE("An idle arrival still settles smoothly onto its tile [core][movement]")
{
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CAPTURE(fps);
        float position[] = {4640.0f, 18940.0f, 215.0f};
        bool active = true;
        const float frameFactor = 25.0f / fps;
        active = UpdateArrivalCatchup(position, 4650.0f, 18950.0f, active, false, frameFactor);
        CHECK(position[0] > 4640.0f);
        CHECK(position[0] < 4650.0f);
        for (int frame = 0; frame < static_cast<int>(fps); ++frame)
            active = UpdateArrivalCatchup(position, 4650.0f, 18950.0f, active, false, frameFactor);
        CHECK_FALSE(active);
        CHECK(position[0] == 4650.0f);
        CHECK(position[1] == 18950.0f);
        CHECK(position[2] == 215.0f);
    }
}

TEST_CASE("A cancelled arrival leaves the new position alone [core][movement]")
{
    float position[] = {19950.0f, 4750.0f, 100.0f};
    CHECK_FALSE(UpdateArrivalCatchup(position, 4650.0f, 18950.0f, false, false, 1.0f));
    CHECK(position[0] == 19950.0f);
    CHECK(position[1] == 4750.0f);
    CHECK(position[2] == 100.0f);
}

TEST_CASE("A server slide takes priority over an old arrival [core][movement]")
{
    float position[] = {4657.0f, 18950.0f, 215.0f};
    CHECK_FALSE(UpdateArrivalCatchup(position, 4650.0f, 18950.0f, true, true, 1.0f));
    CHECK(position[0] == 4657.0f);
    CHECK(position[1] == 18950.0f);
}
