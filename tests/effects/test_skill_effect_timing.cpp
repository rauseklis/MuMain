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

TEST_CASE("Fire Burst homing sub-steps have a finite ceiling")
{
    CHECK(Render::Effects::SkillTiming::FireBurstMaximumGravity == doctest::Approx(5.0f));
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
