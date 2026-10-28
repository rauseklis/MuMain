#pragma once

#include <algorithm>
#include <cmath>

namespace Render::Effects::SkillTiming
{
constexpr int TwistingSlashStageCount = 5;
constexpr float TwistingSlashDriverLifetime = 5.0f;

inline int TwistingSlashStage(float lifetime)
{
    const int elapsedTicks = static_cast<int>(std::floor(TwistingSlashDriverLifetime - lifetime + 0.0001f));
    return std::clamp(elapsedTicks, 0, TwistingSlashStageCount - 1);
}

inline bool CrossedUp(float previous, float current, float threshold)
{
    return previous < threshold && current >= threshold;
}

// Returns the number of legacy 25 Hz events due now, then advances the phase
// by this rendered frame. Set phase to 1 for an event on the first update.
inline int ReferenceEventsDue(float& phase, float frameFactor)
{
    const int due = std::max(0, static_cast<int>(std::floor(phase + 0.0001f)));
    phase -= static_cast<float>(due);
    phase += std::clamp(frameFactor, 0.0f, 1.0f);
    return due;
}
} // namespace Render::Effects::SkillTiming
