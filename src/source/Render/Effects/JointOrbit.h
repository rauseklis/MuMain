#pragma once

#include <cmath>

namespace Render::Effects::JointOrbit
{
// The circling motion of the MODEL_SPEARSKILL aura joints (Soul Barrier SubType 0, the Fenrir/mount
// auras and so on) is a pure function of the world time and the joint's pool slot -- there is no
// integrated state. That is what makes a tail chain reproducible: the position a joint had N frames
// ago is simply this function evaluated at that earlier time. It lives here, extracted unchanged from
// MoveJoint, so the per-frame movement and the tail pre-roll provably use the same math.
struct Direction
{
    float dir[3];
    float sinAdd;
};

inline Direction Evaluate(double worldTimeMs, int jointIndex, float speedScale)
{
    int iFrame = static_cast<int>(worldTimeMs / 40.f);
    iFrame = ((jointIndex % 2) ? iFrame : -iFrame) + jointIndex * 53731;

    const float fSpeed[3] = {0.048f * speedScale, 0.0613f * speedScale, 0.1113f * speedScale};

    float vDirTemp[3];
    vDirTemp[0] = sinf((float)(iFrame + 55555) * fSpeed[0]) * cosf((float)iFrame * fSpeed[1]);
    vDirTemp[1] = sinf((float)(iFrame + 55555) * fSpeed[0]) * sinf((float)iFrame * fSpeed[1]);
    vDirTemp[2] = cosf((float)(iFrame + 55555) * fSpeed[0]);

    Direction result;
    result.sinAdd = sinf((float)(iFrame + 11111) * fSpeed[2]);
    const float fCosAdd = cosf((float)(iFrame + 11111) * fSpeed[2]);
    result.dir[2] = vDirTemp[0];
    result.dir[1] = result.sinAdd * vDirTemp[1] + fCosAdd * vDirTemp[2];
    result.dir[0] = fCosAdd * vDirTemp[1] - result.sinAdd * vDirTemp[2];
    return result;
}

// Offset from the target of a Soul Barrier (SubType 0) joint for a given direction.
inline void SoulBarrierOffset(const Direction& d, float out[3])
{
    out[0] = d.dir[0] * 80.0f;
    out[1] = d.dir[1] * 80.0f;
    out[2] = 110.0f + d.dir[2] * 120.0f;
}

// World time at which the tail point `k` steps back in a chain (k = 0 is the newest point already
// there, so the first pre-rolled one is k = 0 -> one frame ago) would have been recorded by a joint
// updated at a steady `frameMs` cadence (40 ms * FPS_ANIMATION_FACTOR, i.e. 1000 / FPS).
inline double PreRollSampleTimeMs(double nowMs, double frameMs, int k)
{
    return nowMs - (k + 1) * frameMs;
}
} // namespace Render::Effects::JointOrbit
