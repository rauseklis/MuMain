#pragma once

#include <algorithm>

namespace Render::Effects::TailScaling
{
// CreateJoint scales a new effect joint's MaxTails once, at creation, by the current
// FPS_ANIMATION_FACTOR, so a joint's visible tail length looks the same at any frame rate (a
// comment at that call site explains why: tails would otherwise look too short at high FPS). That
// relies on the current frame's FPS reading being representative of real gameplay.
//
// A frame that lands on a stall -- most commonly a map/zone load, which blocks the main loop while
// assets stream in -- can report a raw instantaneous FPS of just a few frames per second, nothing
// like real play even on weak hardware (sustained legacy-cadence play is already handled correctly
// down to 25 FPS by FPS_ANIMATION_FACTOR's own clamp to 1.0). Scaling by such a reading would bake a
// corrupted, far-too-small MaxTails into the joint for its entire lifetime -- joints like Soul
// Barrier's effectively never expire, so the corruption never self-corrects.
//
// 20 FPS is comfortably below any real sustained frame rate this client targets, so a reading under
// it is a reliable signal of a stall rather than genuine low-end play, without being so low that it
// would ever misfire on an actual slow machine.
constexpr double MinReliableFps = 20.0;

// Returns the MaxTails to use right now, given `baseMaxTails` (the type/subtype's un-scaled value)
// and the current frame's `fps`/`fpsAnimationFactor`. When `fps` looks like a stall, `pending` is
// set true and `baseMaxTails` is returned unscaled, so the caller can retry later (once a frame with
// trustworthy timing comes along) via the same function, instead of baking in a guessed value.
inline int ComputeMaxTails(int baseMaxTails, double fps, float fpsAnimationFactor, int maxTailsCap, bool& pending)
{
    if (fps < MinReliableFps)
    {
        pending = true;
        return baseMaxTails;
    }

    pending = false;
    const int scaled = static_cast<int>(baseMaxTails / fpsAnimationFactor);
    return std::min(scaled, maxTailsCap);
}
} // namespace Render::Effects::TailScaling
