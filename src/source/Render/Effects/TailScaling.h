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

// How long after creation a joint that was created during a stall keeps re-evaluating its MaxTails.
// A stall is followed by a recovery ramp (measured live: the FPS counter reads ~1, 29, 60, 108, then
// the steady ~250 over the next several frames), so the first "reliable" reading is still not
// representative -- applying the scaling once on it bakes in a partial value (35/72/130 instead of
// 200). Re-evaluating every frame for a short bounded window lets the value converge instead.
constexpr double RecoveryWindowMs = 2000.0;

// Ratchet: the MaxTails a pending joint should have this frame. Only ever raises `current` toward
// min(cap, base / factor), never lowers it, so a transient dip during the ramp (or one more slow
// frame) cannot pull an already-reached value back down. A player who genuinely sits at a low frame
// rate converges to their real, lower target (factor 1.0 -> base) and simply stays there.
inline int RatchetMaxTails(int current, int baseMaxTails, float fpsAnimationFactor, int maxTailsCap)
{
    const int target = std::min(static_cast<int>(baseMaxTails / fpsAnimationFactor), maxTailsCap);
    return std::max(current, target);
}

// A pending joint is finished once it reached the cap (nothing left to gain) or its bounded
// recovery window has elapsed.
inline bool IsRecoveryDone(int current, int maxTailsCap, double nowMs, double deadlineMs)
{
    return current >= maxTailsCap || nowMs >= deadlineMs;
}
} // namespace Render::Effects::TailScaling
