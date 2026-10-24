#pragma once

#include <algorithm>
#include <cmath>

namespace Physics::ClothSimulationTiming
{
constexpr float LegacyFrame = 1.0f;
constexpr int MaximumCatchUpFrames = 4;

inline int ConsumeLegacyFrames(float frameScale, float& accumulator, bool& started)
{
    if (!started)
    {
        started = true;
        return 1;
    }

    accumulator += std::max(frameScale, 0.0f);

    const int availableFrames = static_cast<int>(std::floor(accumulator / LegacyFrame));
    const int framesToSimulate = std::min(availableFrames, MaximumCatchUpFrames);
    accumulator -= static_cast<float>(framesToSimulate) * LegacyFrame;

    if (availableFrames > MaximumCatchUpFrames)
    {
        accumulator = std::fmod(accumulator, LegacyFrame);
    }

    return framesToSimulate;
}

// How far (0..1) the render is between the last simulated step and the next one: what is left in the accumulator.
inline float InterpolationAlpha(float accumulator)
{
    return std::clamp(accumulator / LegacyFrame, 0.0f, 1.0f);
}

// The most an owner may move between two frames before the cloth no longer follows it but is thrown away (and made
// again where the owner is). A teleport, a respawn at another spot or a long dash would otherwise leave the free
// vertices behind and stretch the cloth across the map.
constexpr float OwnerJumpDistance = 600.0f;

inline bool OwnerJumped(const float previous[3], const float current[3], float maximumDistance = OwnerJumpDistance)
{
    const float dx = current[0] - previous[0];
    const float dy = current[1] - previous[1];
    const float dz = current[2] - previous[2];
    return dx * dx + dy * dy + dz * dz > maximumDistance * maximumDistance;
}

inline float Lerp(float previous, float current, float alpha)
{
    return previous + (current - previous) * alpha;
}
}
