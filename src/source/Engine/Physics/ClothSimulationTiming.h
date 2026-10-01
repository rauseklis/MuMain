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
}
