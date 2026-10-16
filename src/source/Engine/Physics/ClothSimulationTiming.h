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

inline float Lerp(float previous, float current, float alpha)
{
    return previous + (current - previous) * alpha;
}
}
