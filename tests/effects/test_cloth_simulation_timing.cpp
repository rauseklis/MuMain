#include "doctest.h"

#include "Engine/Physics/ClothSimulationTiming.h"

TEST_CASE("cloth simulation retains its legacy cadence at high render rates")
{
    float accumulator = 0.0f;
    bool started = false;
    int simulatedFrames = 0;

    for (int renderFrame = 0; renderFrame < 240; ++renderFrame)
    {
        simulatedFrames += Physics::ClothSimulationTiming::ConsumeLegacyFrames(
            25.0f / 240.0f,
            accumulator,
            started);
    }

    CHECK(simulatedFrames == 25);
    CHECK(accumulator == doctest::Approx(239.0f * 25.0f / 240.0f - 24.0f));
}

TEST_CASE("cloth simulation advances once per render at the legacy frame rate")
{
    float accumulator = 0.0f;
    bool started = false;

    for (int renderFrame = 0; renderFrame < 25; ++renderFrame)
    {
        CHECK(Physics::ClothSimulationTiming::ConsumeLegacyFrames(1.0f, accumulator, started) == 1);
    }
}

TEST_CASE("cloth simulation caps recovery after a long frame")
{
    float accumulator = 0.0f;
    bool started = true;

    CHECK(Physics::ClothSimulationTiming::ConsumeLegacyFrames(12.5f, accumulator, started) == 4);
    CHECK(accumulator == doctest::Approx(0.5f));
}

TEST_CASE("cloth interpolation alpha is the fraction of the next step that has elapsed")
{
    CHECK(Physics::ClothSimulationTiming::InterpolationAlpha(0.0f) == doctest::Approx(0.0f));
    CHECK(Physics::ClothSimulationTiming::InterpolationAlpha(0.25f) == doctest::Approx(0.25f));
    CHECK(Physics::ClothSimulationTiming::InterpolationAlpha(1.5f) == doctest::Approx(1.0f));
    CHECK(Physics::ClothSimulationTiming::InterpolationAlpha(-0.5f) == doctest::Approx(0.0f));
}

TEST_CASE("cloth interpolation blends between the previous and the current position")
{
    CHECK(Physics::ClothSimulationTiming::Lerp(10.0f, 20.0f, 0.0f) == doctest::Approx(10.0f));
    CHECK(Physics::ClothSimulationTiming::Lerp(10.0f, 20.0f, 0.5f) == doctest::Approx(15.0f));
    CHECK(Physics::ClothSimulationTiming::Lerp(10.0f, 20.0f, 1.0f) == doctest::Approx(20.0f));
    CHECK(Physics::ClothSimulationTiming::Lerp(-4.0f, 4.0f, 0.25f) == doctest::Approx(-2.0f));
}
