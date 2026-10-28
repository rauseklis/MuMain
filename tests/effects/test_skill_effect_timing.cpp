#include "doctest.h"

#include <algorithm>
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
} // namespace

TEST_CASE("Twisting Slash emits every canonical trail stage at every frame rate")
{
    const std::vector<int> expected = {0, 1, 2, 3, 4};
    for (const float fps : {25.0f, 60.0f, 120.0f, 240.0f})
    {
        CHECK(SimulateTwistingSlash(fps) == expected);
    }
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
