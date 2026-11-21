#include "AttackFollowUp.h"

namespace MUHelper
{

void AttackFollowUp::OnAttack(bool landed, bool swingInProgress)
{
    m_landed = landed;
    m_swingStarted = landed && swingInProgress;
}

void AttackFollowUp::Clear()
{
    m_landed = false;
    m_swingStarted = false;
}

bool AttackFollowUp::IsDue(bool swingInProgress)
{
    if (!m_landed)
    {
        return false;
    }

    if (swingInProgress)
    {
        m_swingStarted = true;
        return false;
    }

    return m_swingStarted;
}

} // namespace MUHelper
