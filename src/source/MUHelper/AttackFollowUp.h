#pragma once

namespace MUHelper
{

// Decides when the MU Helper may attack again between two full work runs. After a hit the next attack is tried as
// soon as the swing has ended, like holding the mouse button does. It follows up only an attack that landed and whose
// swing was seen to start: walking to a target, a refused skill, no target, or a cast that shows no swing waits for
// the next full work, so nothing is sent to the server faster than the swings allow.
class AttackFollowUp
{
public:
    // Call after every attack attempt that was not refused for a swing still in progress.
    void OnAttack(bool landed, bool swingInProgress);

    // Call when nothing may attack (no target, or a higher-priority helper action such as pickup or walking back).
    void Clear();

    // True when the next attack may be tried now.
    [[nodiscard]] bool IsDue(bool swingInProgress);

private:
    bool m_landed = false;
    bool m_swingStarted = false;
};

} // namespace MUHelper
