#pragma once

#include <algorithm>
#include <cmath>

extern float FPS_ANIMATION_FACTOR;

namespace mu
{
template <typename T>
[[nodiscard]] constexpr T FrameScale(T value, float factor)
{
    return value * static_cast<T>(factor);
}

template <typename T>
[[nodiscard]] T FrameDecay(T base, float factor)
{
    return static_cast<T>(std::pow(static_cast<double>(base), static_cast<double>(factor)));
}

template <typename T>
[[nodiscard]] T FrameStepTowards(T current, T target, T step, float factor)
{
    const T scaledStep = FrameScale(step, factor);
    if (current < target)
    {
        return std::min<T>(current + scaledStep, target);
    }
    if (current > target)
    {
        return std::max<T>(current - scaledStep, target);
    }
    return current;
}

template <typename T>
[[nodiscard]] T FrameLerp(T current, T target, T referenceRate, float factor)
{
    const T clampedRate = std::clamp<T>(referenceRate, static_cast<T>(0), static_cast<T>(1));
    const T scaledRate = static_cast<T>(1) - FrameDecay(static_cast<T>(1) - clampedRate, factor);
    return current + (target - current) * scaledRate;
}

template <typename T>
[[nodiscard]] T FrameScale(T value)
{
    return FrameScale(value, FPS_ANIMATION_FACTOR);
}

template <typename T>
[[nodiscard]] T FrameDecay(T base)
{
    return FrameDecay(base, FPS_ANIMATION_FACTOR);
}

template <typename T>
[[nodiscard]] T FrameStepTowards(T current, T target, T step)
{
    return FrameStepTowards(current, target, step, FPS_ANIMATION_FACTOR);
}

template <typename T>
[[nodiscard]] T FrameLerp(T current, T target, T referenceRate)
{
    return FrameLerp(current, target, referenceRate, FPS_ANIMATION_FACTOR);
}
}

