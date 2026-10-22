#pragma once

#include <algorithm>
#include <cmath>

// How the trail of a swinging weapon is laid out each rendered frame.
//
// The animation advances by PlaySpeed * frameFactor each rendered frame (frameFactor is REFERENCE_FPS / FPS, at most 1).
// The trail of one frame has to cover exactly that advance, so the points of the next frame start where the points of
// this one ended. Laying a full PlaySpeed slice every frame (as an earlier version did) makes the slices overlap at a
// high frame rate: the points then run backwards and forwards along the swing and the trail turns into a thick, solid
// mass instead of a thin streak.
namespace Render::Effects::WeaponTrail
{
    struct Slice
    {
        float firstOffset; // from the current animation frame, always <= 0
        float step;        // animation frames between two points
        int count;         // number of points
    };

    // maxPoints is the number of points a trail gets at the reference frame rate.
    inline Slice SliceForFrame(float playSpeed, float frameFactor, int maxPoints)
    {
        const float factor = std::clamp(frameFactor, 0.0f, 1.0f);
        const float advance = playSpeed * factor;
        const int count = std::clamp(static_cast<int>(std::ceil(static_cast<float>(maxPoints) * factor)), 1, maxPoints);
        return {-advance, advance / static_cast<float>(count), count};
    }
}
