#include "doctest.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "Render/Effects/SkillEffectTiming.h"

namespace
{
constexpr float ReferenceFps = 25.0f;

std::vector<int> SimulateTwistingSlash(float fps)
{
    const float factor = std::min(ReferenceFps / fps, 1.0f);
    float lifetime = Render::Effects::SkillTiming::TwistingSlashDriverLifetime;
    int nextStage = 0;
    std::vector<int> stages;
    while (lifetime > 0.0f)
    {
        const int currentStage = Render::Effects::SkillTiming::TwistingSlashStage(lifetime);
        while (nextStage <= currentStage)
        {
            stages.push_back(nextStage++);
        }
        lifetime -= factor;
    }
    return stages;
}

int CountReferenceEvents(float fps, float seconds)
{
    const float factor = std::min(ReferenceFps / fps, 1.0f);
    float phase = 1.0f;
    int count = 0;
    for (int frame = 0; frame < static_cast<int>(fps * seconds); ++frame)
    {
        count += Render::Effects::SkillTiming::ReferenceEventsDue(phase, factor);
    }
    return count;
}

int CountDescendingMilestone(float fps, float start, float threshold)
{
    const float factor = std::min(ReferenceFps / fps, 1.0f);
    float lifetime = start;
    int count = 0;
    while (lifetime > threshold - 1.0f)
    {
        count += Render::Effects::SkillTiming::CrossedDown(lifetime, factor, threshold) ? 1 : 0;
        lifetime -= factor;
    }
    return count;
}

bool SimulateRagefulBlow(float fps)
{
    const float factor = std::min(ReferenceFps / fps, 1.0f);
    float lifetime = 20.0f;
    float gravity = 50.0f;
    bool reversed = false;
    while (lifetime > 10.0f)
    {
        if (Render::Effects::SkillTiming::CrossedDown(lifetime, factor, 15.0f))
        {
            gravity = -gravity;
            reversed = true;
        }

        if (lifetime > 9.0f && lifetime < 16.0f)
        {
            gravity -= 8.0f * factor;
        }
        else
        {
            gravity += 8.0f * factor;
        }

        if (!std::isfinite(gravity))
        {
            return false;
        }
        lifetime -= factor;
    }
    return reversed && gravity < 0.0f;
}
} // namespace

TEST_CASE("Twisting Slash emits every canonical trail stage at every frame rate")
{
    const std::vector<int> expected = {0, 1, 2, 3, 4};
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CHECK(SimulateTwistingSlash(fps) == expected);
    }
}

TEST_CASE("Twisting Slash uses the reviewed compact orbit while retaining the spear proportion")
{
    using Render::Effects::SkillTiming::TwistingSlashRadius;
    CHECK(TwistingSlashRadius(false) == doctest::Approx(112.5f));
    CHECK(TwistingSlashRadius(true) == doctest::Approx(135.0f));
    CHECK(TwistingSlashRadius(true) / TwistingSlashRadius(false) == doctest::Approx(1.2f));
}

TEST_CASE("reference-tick skill emissions retain their 25 Hz cadence")
{
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CHECK(CountReferenceEvents(fps, 1.0f) == 25);
    }
}

TEST_CASE("animation thresholds fire once when crossed")
{
    using Render::Effects::SkillTiming::CrossedUp;
    CHECK_FALSE(CrossedUp(2.0f, 2.9f, 3.0f));
    CHECK(CrossedUp(2.9f, 3.1f, 3.0f));
    CHECK_FALSE(CrossedUp(3.1f, 3.4f, 3.0f));
}

TEST_CASE("descending effect milestones fire exactly once at every frame rate")
{
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CHECK(CountDescendingMilestone(fps, 20.0f, 15.0f) == 1);
        CHECK(CountDescendingMilestone(fps, 20.0f, 13.0f) == 1);
        CHECK(CountDescendingMilestone(fps, 20.0f, 11.0f) == 1);
        CHECK(CountDescendingMilestone(fps, 20.0f, 10.0f) == 1);
    }
}

TEST_CASE("Rageful Blow completes a finite rise and return at every frame rate")
{
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CHECK(SimulateRagefulBlow(fps));
    }
}

namespace
{
// What the legacy handler saw at 25 FPS: every whole lifetime value, once, from the start down to 1.
std::vector<int> SimulateLifetimeTicks(float fps, float startLifetime)
{
    const float factor = std::min(ReferenceFps / fps, 1.0f);
    float lifetime = startLifetime;
    std::vector<int> ticks;
    while (lifetime > 0.0f)
    {
        const int tick = Render::Effects::SkillTiming::LifetimeTickCrossed(lifetime, factor);
        if (tick > 0)
        {
            ticks.push_back(tick);
        }
        lifetime -= factor;
    }
    return ticks;
}
} // namespace

TEST_CASE("Lifetime ticks are reported once each at every frame rate")
{
    std::vector<int> legacy;
    for (int tick = 14; tick >= 1; --tick)
    {
        legacy.push_back(tick);
    }

    for (const float fps : {25.0f, 60.0f, 120.0f, 144.0f, 165.0f, 240.0f, 360.0f})
    {
        CHECK(SimulateLifetimeTicks(fps, 14.0f) == legacy);
    }
}

TEST_CASE("Multi Shot's blade fires its six even ticks once each at every frame rate")
{
    // Move_MODEL_BLADE_SKILL: `LifeTime >= 3 && (int)LifeTime % 2 == 0` created one arrow per hit.
    for (const float fps : {25.0f, 144.0f, 240.0f})
    {
        int arrows = 0;
        for (const int tick : SimulateLifetimeTicks(fps, 14.0f))
        {
            arrows += (tick >= 3 && tick % 2 == 0) ? 1 : 0;
        }
        CHECK(arrows == 6);
    }
}

TEST_CASE("An arrow's end-of-life explosion happens once at every frame rate")
{
    // Move_MODEL_ARROW_BOMB / ARROW_DRILL: `LifetimeTickCrossed(...) == 1` creates the explosion.
    for (const float fps : {25.0f, 144.0f, 240.0f})
    {
        int explosions = 0;
        for (const int tick : SimulateLifetimeTicks(fps, 40.0f))
        {
            explosions += tick == 1 ? 1 : 0;
        }
        CHECK(explosions == 1);
    }
}

TEST_CASE("Multi Shot covers its whole fan on every cast at every frame rate")
{
    // Each of the blade's six arrows takes a fixed direction by tick, so a cast never leaves a gap.
    for (const float fps : {25.0f, 144.0f, 240.0f})
    {
        std::vector<float> offsets;
        for (const int tick : SimulateLifetimeTicks(fps, 14.0f))
        {
            if (tick >= 3 && tick % 2 == 0)
            {
                offsets.push_back(Render::Effects::SkillTiming::MultiShotFanOffset(tick));
            }
        }
        std::sort(offsets.begin(), offsets.end());
        const std::vector<float> fan = {-35.0f, -21.0f, -7.0f, 7.0f, 21.0f, 35.0f};
        CHECK((offsets == fan));
    }
}

TEST_CASE("Multi Shot's fan alternates sides, starting close to the aim")
{
    using Render::Effects::SkillTiming::MultiShotFanOffset;
    CHECK(MultiShotFanOffset(14) == 7.0f);
    CHECK(MultiShotFanOffset(12) == -7.0f);
    CHECK(MultiShotFanOffset(10) == 21.0f);
    CHECK(MultiShotFanOffset(8) == -21.0f);
    CHECK(MultiShotFanOffset(6) == 35.0f);
    CHECK(MultiShotFanOffset(4) == -35.0f);
}
