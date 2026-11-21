#pragma once

#include <cstdint>

namespace MUHelper
{

// Paces the MU Helper. The helper timer fires often so a new attack can start as soon as the previous swing ends,
// like holding the mouse button does. Everything else the helper does (buffs, potions, pickup, repair, walking back)
// still runs only once per full work interval, so those requests are not sent to the server more often than before.
// Seconds are counted from the real clock, so skill and buff timers set in seconds last real seconds.
class HelperClock
{
public:
    // How often the helper timer fires.
    static constexpr unsigned TickMs = 50;
    // How often the full helper work runs.
    static constexpr std::uint64_t FullWorkIntervalMs = 250;

    void Start(std::uint64_t nowMs);

    // True when the full helper work is due; the next one is then a full interval later.
    [[nodiscard]] bool TakeFullWorkDue(std::uint64_t nowMs);

    // Whole seconds since Start.
    [[nodiscard]] int SecondsElapsed(std::uint64_t nowMs) const;

private:
    std::uint64_t m_startMs = 0;
    std::uint64_t m_lastFullWorkMs = 0;
    bool m_fullWorkDone = false;
};

} // namespace MUHelper
