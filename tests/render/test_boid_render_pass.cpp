#include <doctest.h>

#include "Engine/AI/BoidRenderPass.h"

TEST_CASE("An invisible newly spawned boid cannot submit an unprepared shadow [render][boid]")
{
    bool bodyAttempted = false;
    bool shadowSubmitted = false;
    Engine::AI::RenderBoidPass(
        [&]
        {
            bodyAttempted = true;
            return false;
        },
        [&] { shadowSubmitted = true; });
    CHECK(bodyAttempted);
    CHECK_FALSE(shadowSubmitted);
}

TEST_CASE("A visible boid prepares its model before drawing effects and shadow [render][boid]")
{
    bool palettePrepared = false;
    int shadowCount = 0;
    Engine::AI::RenderBoidPass(
        [&]
        {
            palettePrepared = true;
            return true;
        },
        [&]
        {
            CHECK(palettePrepared);
            ++shadowCount;
        });
    CHECK(shadowCount == 1);
}

TEST_CASE("A faded boid cannot reuse another object's model palette [render][boid]")
{
    int paletteOwner = 1;
    int shadowOwner = -1;
    Engine::AI::RenderBoidPass([] { return false; }, [&] { shadowOwner = paletteOwner; });
    CHECK(shadowOwner == -1);
}
