# Kalima: apocalyptic look

All seven Kalimas share one map (`Data/World25`, objects in `Data/Object25`), so one set of textures changes them all. The map shape is
unchanged; only textures were swapped.

- **Ground:** the Lost Tower stone tiles (`World5`) replace the blue brick and sand. Where Lost Tower's own tile is blue ice or a blue
  floor, a neutral stone tile is used instead.
- **Water and waterfalls:** lava from the Vulcanus-style maps (`World64` lava tile, `Object64` `song_lava1` and `MagmaFish02`) replaces the
  water, the waterfall textures and the blue glows. The `@2x` HD copies were replaced too, otherwise they would still show the old look.
- Textures are looked up by file name, so a texture can be swapped by replacing the same file name in the map's folder (and its `@2x` copy).

**HD copies must match.** The game prefers a picture in `World25@2x` over the normal file of the same name (the main ground tile is the lowercase
`tilegrass01.jpg.png`). Replacing only the normal file changes nothing in game. The HD picture must be exactly twice the size of the normal
one, otherwise it is rejected (the log says "HD texture rejected").
