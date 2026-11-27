# Kalima: the burning look

All seven Kalimas share one map (`Data/World25`, objects in `Data/Object25`), so one set of changes covers them all.
The shape of the map and everything the server cares about (walkable areas, safe zones, gates, monsters) are unchanged.

## What players see

- **Ground:** a dark charcoal floor split into irregular regions of different ground: sparse glowing lava cracks, dark rock,
  rubble, dry cracked earth, obsidian and pale ash. Ash-dust patches sit around groups of objects so they stand out.
  Different kinds fade through plain floor where they meet.
- **Lava river:** the lake is a detailed lava tile (dark crust with glowing splits) with a soft edge.
- **Walls and stone:** burnt ash-brown instead of blue-green; the animated water ripple that used to lie over them is gone.
- **Waterfalls:** slow, dark-red lava blobs instead of fast white foam (Kalima only).
- **Monsters:** sparks and embers instead of water smoke and bubbles; the Aegis, Death Centurion and Necron textures are
  fire and ash coloured; blue glows became ember orange.
- **Air:** ember sparks fly up around the player.
- **Light:** the map's light is red and darker (`TerrainLight`).

## How it is made (for whoever edits it next)

- Ground textures are looked up by file name in `Data/World25` (256 px `.OZJ`) and need a matching `Data/World25@2x/<name>.png` of
  exactly twice the size, otherwise the HD copy is rejected (the game log says "HD texture rejected"). The HD copy wins over the
  normal file, so changing only the normal file shows nothing.
- `Data/World25/EncTerrain25.map` is the client-only ground painting: one base tile plus **one** second tile per square, blended by
  a strength per corner point. The game cannot blend two different second tiles in one square, so different ground kinds must
  fade out to plain floor where they meet, otherwise hard-edged blocks appear.
- The server reads only the walk and safe-zone flags (`Terrain25.att` in the server repository), never the textures or the
  `.map`, so none of this needs a server change. A change to the walkable layout would need the server file changed too.
- Kalima-only code is in `World/GameMaps/GMHellas.cpp` (colours, sparks, waterfall tint) and one new mesh effect in `ZzzBMD.cpp`.

## Notes

- The original Kalima sounds are not in the repository; a client without `Data/Sound` has no sound at all.
