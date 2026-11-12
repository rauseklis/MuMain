#pragma once

#include <algorithm>
#include <cmath>

namespace Render::Effects::SkillTiming
{
constexpr int TwistingSlashStageCount = 5;
constexpr float TwistingSlashDriverLifetime = 5.0f;

inline float TwistingSlashRadius(bool usesSpear)
{
    // The legacy 150/180-unit orbit overwhelms the character at the modern
    // camera scale. Keep the spear proportion while tightening both rings.
    return usesSpear ? 135.0f : 112.5f;
}

inline int TwistingSlashStage(float lifetime)
{
    const int elapsedTicks = static_cast<int>(std::floor(TwistingSlashDriverLifetime - lifetime + 0.0001f));
    return std::clamp(elapsedTicks, 0, TwistingSlashStageCount - 1);
}

inline bool CrossedUp(float previous, float current, float threshold)
{
    return previous < threshold && current >= threshold;
}

// Effect lifetimes count down by one legacy tick at 25 Hz and by a fractional
// tick at higher render rates. This detects the one rendered frame which
// crosses a legacy lifetime milestone without relying on an integer cast.
inline bool CrossedDown(float current, float frameFactor, float threshold)
{
    constexpr float epsilon = 0.0001f;
    const float step = std::clamp(frameFactor, 0.0f, 1.0f);
    return current <= threshold + epsilon && current + step > threshold + epsilon;
}

// The whole lifetime value this rendered frame passes, or a value below 1 when it passes none.
// Legacy handlers ran `(int)o->LifeTime == n` or `% n` once per 25 Hz tick, on lifetimes that were
// whole numbers. At higher rates the lifetime is fractional and keeps one integer part for several
// frames, so the same check fires on every one of them: Multi Shot launched an arrow on each. Call
// this where the handler runs (before the lifetime is reduced) and test the returned tick instead;
// at 25 FPS it equals `(int)lifeTime`.
inline int LifetimeTickCrossed(float lifeTime, float frameFactor)
{
    constexpr float epsilon = 0.0001f;
    const float step = std::clamp(frameFactor, 0.0f, 1.0f);
    const int tick = static_cast<int>(std::ceil(lifeTime - epsilon));
    return lifeTime + step > static_cast<float>(tick) + epsilon ? tick : 0;
}

inline float CardinalAngle(int index)
{
    return static_cast<float>((index % 4) * 90);
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
