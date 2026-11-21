#include "HelperClock.h"

namespace MUHelper
{

namespace
{
constexpr std::uint64_t MsPerSecond = 1000;
}

void HelperClock::Start(std::uint64_t nowMs)
{
    m_startMs = nowMs;
    m_lastFullWorkMs = nowMs;
    m_fullWorkDone = false;
}

bool HelperClock::TakeFullWorkDue(std::uint64_t nowMs)
{
    // The first tick after Start works at once, as the old timer did on its first tick.
    if (m_fullWorkDone && nowMs - m_lastFullWorkMs < FullWorkIntervalMs)
    {
        return false;
    }

    m_fullWorkDone = true;
    m_lastFullWorkMs = nowMs;
    return true;
}

int HelperClock::SecondsElapsed(std::uint64_t nowMs) const
{
    if (nowMs <= m_startMs)
    {
        return 0;
    }

    return static_cast<int>((nowMs - m_startMs) / MsPerSecond);
}

} // namespace MUHelper
