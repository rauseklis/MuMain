// client/src/source/GameLogic/ExpHunt/ExpHuntTracker.h
//
// Tracks a trailing-60-second experience-per-minute rate for the /hunt
// player command. Pure logic, no UI or networking knowledge - fed by
// whichever packet handler currently receives experience gains.
#pragma once

#include <cstdint>

namespace GameLogic::ExpHunt
{
    // Clears all accumulated samples and marks the tracker active. Safe to
    // call again while already active (restarts from zero).
    void Start();

    // Marks the tracker inactive and clears all accumulated samples.
    void Stop();

    bool IsActive();

    // Records one experience-gain sample at the current time. No-op while
    // inactive. A single kill can arrive as multiple samples (the server
    // splits large per-kill exp values across multiple packets), so every
    // call must be summed - never assume one call equals one kill.
    void AddSample(std::uint64_t experience);

    // Experience-per-minute rate over the trailing 60-second window,
    // extrapolated from actual elapsed time when tracking started less than
    // 60 seconds ago. Returns 0.0 while inactive or before any sample has
    // been recorded.
    double GetRatePerMinute();
}
