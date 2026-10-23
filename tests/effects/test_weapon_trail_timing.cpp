#include "doctest.h"

#include <algorithm>
#include <vector>

#include "Render/Effects/WeaponTrailTiming.h"

namespace
{
constexpr float PLAY_SPEED = 0.55f;
constexpr int REFERENCE_POINTS = 10;
constexpr float REFERENCE_FPS = 25.0f;

// Runs one swing at the given frame rate and returns the animation frame of every trail point, in the order they are added.
std::vector<float> SimulateSwing(float fps, float seconds)
{
    const float factor = std::min(REFERENCE_FPS / fps, 1.0f);
    std::vector<float> points;
    float animationFrame = 3.0f;
    for (int frame = 0; frame < static_cast<int>(seconds * fps); ++frame)
    {
        animationFrame += PLAY_SPEED * factor;
        const auto slice = Render::Effects::WeaponTrail::SliceForFrame(PLAY_SPEED, factor, REFERENCE_POINTS);
        for (int i = 0; i < slice.count; ++i)
        {
            points.push_back(animationFrame + slice.firstOffset + slice.step * static_cast<float>(i));
        }
    }
    return points;
}
}

TEST_CASE("a weapon swing never runs backwards along its path, at any frame rate")
{
    for (const float fps : {20.0f, 25.0f, 30.0f, 60.0f, 144.0f, 240.0f})
    {
        const auto points = SimulateSwing(fps, 0.6f);
        REQUIRE(points.size() > 1);
        for (size_t i = 1; i < points.size(); ++i)
        {
            CHECK(points[i] >= points[i - 1] - 1e-4f);
        }
    }
}

TEST_CASE("the points of one second cover the same stretch of the swing at every frame rate")
{
    const float expectedSpan = PLAY_SPEED * REFERENCE_FPS;
    for (const float fps : {25.0f, 60.0f, 144.0f})
    {
        const auto points = SimulateSwing(fps, 1.0f);
        CHECK(points.back() - points.front() == doctest::Approx(expectedSpan).epsilon(0.1));
    }
}

TEST_CASE("the old fixed slice overlapped itself at a high frame rate")
{
    // Documents the bug: every frame laid a full PlaySpeed slice, but the animation had advanced only a fraction of it.
    const float factor = REFERENCE_FPS / 60.0f;
    const float advance = PLAY_SPEED * factor;
    const float previousEnd = 10.0f;
    const float nextStart = previousEnd + advance - PLAY_SPEED;
    CHECK(nextStart < previousEnd);
}

TEST_CASE("the trail of a frame is never empty")
{
    const auto slice = Render::Effects::WeaponTrail::SliceForFrame(PLAY_SPEED, 0.01f, REFERENCE_POINTS);
    CHECK(slice.count >= 1);
}
