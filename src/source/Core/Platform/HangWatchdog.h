#pragma once

namespace Core::Platform::HangWatchdog
{
// Call once per main-loop iteration. The first call also starts the watchdog thread.
// If the loop stops calling for a few seconds (the game is frozen), the watchdog writes
// where the main thread is stuck (module and offset of each stack frame) to MuError.log.
// Offsets can be resolved against the build's Main.map or Main.pdb.
#ifdef _WIN32
void Beat();
#else
inline void Beat() {}
#endif
} // namespace Core::Platform::HangWatchdog
