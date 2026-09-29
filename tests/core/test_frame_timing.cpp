#include "doctest.h"

#include "Core/Time/FrameTiming.h"

#include <cmath>

TEST_CASE("FrameScale preserves 25 FPS reference movement at higher FPS")
{
    constexpr float referenceStep = 10.0f;

    CHECK(mu::FrameScale(referenceStep, 1.0f) == doctest::Approx(10.0f));
    CHECK(mu::FrameScale(referenceStep, 25.0f / 50.0f) == doctest::Approx(5.0f));
    CHECK(mu::FrameScale(referenceStep, 25.0f / 240.0f) == doctest::Approx(1.0416666f));
}

TEST_CASE("FrameStepTowards scales linear fades without overshooting")
{
    CHECK(mu::FrameStepTowards(0.0f, 1.0f, 0.05f, 1.0f) == doctest::Approx(0.05f));
    CHECK(mu::FrameStepTowards(0.0f, 1.0f, 0.05f, 25.0f / 100.0f) == doctest::Approx(0.0125f));
    CHECK(mu::FrameStepTowards(0.99f, 1.0f, 0.05f, 1.0f) == doctest::Approx(1.0f));
    CHECK(mu::FrameStepTowards(0.01f, 0.0f, 0.05f, 1.0f) == doctest::Approx(0.0f));
}

TEST_CASE("FrameDecay preserves exponential per-reference-frame decay over one second")
{
    constexpr float base = 0.9f;
    const float at25Fps = std::pow(mu::FrameDecay(base, 1.0f), 25.0f);
    const float at100Fps = std::pow(mu::FrameDecay(base, 25.0f / 100.0f), 100.0f);
    const float at240Fps = std::pow(mu::FrameDecay(base, 25.0f / 240.0f), 240.0f);

    CHECK(at100Fps == doctest::Approx(at25Fps).epsilon(0.0001));
    CHECK(at240Fps == doctest::Approx(at25Fps).epsilon(0.0001));
}

TEST_CASE("FrameLerp preserves exponential interpolation curve over one second")
{
    constexpr float target = 1.0f;
    constexpr float referenceRate = 0.1f;

    float at25Fps = 0.0f;
    for (int i = 0; i < 25; ++i)
    {
        at25Fps = mu::FrameLerp(at25Fps, target, referenceRate, 1.0f);
    }

    float at100Fps = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        at100Fps = mu::FrameLerp(at100Fps, target, referenceRate, 25.0f / 100.0f);
    }

    float at240Fps = 0.0f;
    for (int i = 0; i < 240; ++i)
    {
        at240Fps = mu::FrameLerp(at240Fps, target, referenceRate, 25.0f / 240.0f);
    }

    CHECK(at100Fps == doctest::Approx(at25Fps).epsilon(0.0001));
    CHECK(at240Fps == doctest::Approx(at25Fps).epsilon(0.0001));
}

