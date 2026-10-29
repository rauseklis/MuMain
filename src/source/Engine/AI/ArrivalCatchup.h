#pragma once

namespace Engine::Movement
{
// Returns whether the remaining arrival correction is still active.
bool UpdateArrivalCatchup(float position[3], float targetX, float targetY, bool active, bool moving, float frameFactor);
} // namespace Engine::Movement
