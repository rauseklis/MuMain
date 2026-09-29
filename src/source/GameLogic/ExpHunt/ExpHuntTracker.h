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

    // Sum of experience gained in the trailing 60-second window. Rises when a
    // new sample is added, falls only when a sample ages past 60 seconds -
    // stays static otherwise. No extrapolation: during the first 60 seconds
    // of tracking this under-represents the eventual steady-state rate (not
    // enough of the window has been observed yet), which is expected - hunt
    // a spot for at least a minute before comparing it to another. Returns
    // 0.0 while inactive or before any sample has been recorded.
    double GetRatePerMinute();
}
