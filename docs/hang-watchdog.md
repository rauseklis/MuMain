# Hang watchdog

If the game stops finishing frames for 8 seconds (it is frozen, "Not responding"), a small background thread writes where the
main thread is stuck into `MuError.log`:

```
[Hang] No frame for 8 s. Main thread stack (module+offset):
  #0 Main.exe+0x1234AB
  #1 nvoglv64.dll+0x...
```

It repeats up to three times, ten seconds apart, so a stuck-for-good freeze shows the same frames each time. A long but normal
pause (for example a big map load) can also trigger it once; that is harmless.

To read the offsets, open the build's `Main.map` (next to `Main.exe`; the offset is the RVA) and find the function that contains
it. Frames in a driver or system DLL (for example the graphics driver) mean the game was waiting inside that DLL.

The watchdog costs one atomic counter increment per frame and a thread that wakes once a second.
