#include "ArrivalCatchup.h"

#include <cmath>

namespace Engine::Movement
{
bool UpdateArrivalCatchup(float position[3], float targetX, float targetY, bool active, bool moving, float frameFactor)
{
    // Helper approaches install short paths directly, bypassing PathFinding2's
    // cancellation. Never let an old arrival pull against a new walk or slide.
    if (!active || moving)
        return false;

    const float dx = targetX - position[0];
    const float dy = targetY - position[1];
    constexpr float SnapEpsilon = 0.5f;
    if (dx * dx + dy * dy <= SnapEpsilon * SnapEpsilon)
    {
        position[0] = targetX;
        position[1] = targetY;
        return false;
    }

    // Preserve the existing arrival easing: about 95% in 60 milliseconds.
    constexpr float CatchupSeconds = 0.060f;
    constexpr float TimeConstant = CatchupSeconds / 3.0f;
    constexpr float ReferenceFps = 25.0f;
    const float dt = frameFactor / ReferenceFps;
    const float blend = 1.0f - std::exp(-dt / TimeConstant);
    position[0] += dx * blend;
    position[1] += dy * blend;
    return true;
}
} // namespace Engine::Movement
