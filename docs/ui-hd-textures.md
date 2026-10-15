# HD interface textures

The in-game interface is drawn from very small textures (the HP orb is 45 x 39). On a modern display they are stretched
1.6x to 4x and look soft. Interface textures can now have an optional high-resolution replacement that shows the
**same picture** with more texels. Nothing about layout, colours or behaviour changes.

## For players

`config.ini`:

```ini
[UI]
HdTextures=auto
```

| Value | Meaning |
|---|---|
| `auto` (default) | Use the 4x art where it exists, otherwise 2x, otherwise the original. |
| `4` | Same as `auto` (4x, then 2x, then original). |
| `2` | Use only the 2x art. Lighter on memory. |
| `off` | Always use the original textures (the classic look). |

The setting is read at start-up. Restart the game after changing it. A missing HD file is never an error: that one
texture simply keeps its original look.

HD art uses about four to sixteen times the texture memory of the original, so weaker machines can use `2` or `off`.
HD textures are mipmapped (it follows `[Render] TextureMipmaps`), so a 4x texture shown smaller than its native size
stays smooth instead of shimmering.

## Where the files live

```
src/bin/Data/Interface/newui_item_back01.OZT          original (always present)
src/bin/Data/Interface@4x/newui_item_back01.png       4x master
src/bin/Data/Interface@2x/newui_item_back01.png       2x, reduced from the 4x master
```

The HD folders mirror the original path and file name; only the extension is `.png`. The 4x PNGs are the masters under
version control; the 2x folder is always generated from them.

A HD file is accepted only when its size is an exact whole-number multiple (2, 3 or 4) of the original in both
directions (for example a 190 x 64 original needs 760 x 256 at 4x). Anything else is ignored and logged as a warning.
Textures that came from a JPEG (`.OZJ`) stay fully opaque, like the original.

## Making or changing HD art

Needs Python 3 with Pillow and numpy, plus the Real-ESRGAN ncnn build (a small standalone Vulkan program, not part of
the repository). Download `realesrgan-ncnn-vulkan-20220424-windows.zip` from the Real-ESRGAN releases page on GitHub,
and copy `realesrgan-ncnn-vulkan.exe`, `vcomp140.dll` and its `models` folder into `tools/ui_hd/.esrgan/` (that folder
is git-ignored), or set the `UI_HD_ESRGAN` environment variable to the exe. Without it use `--method lanczos`.

```
python tools/ui_hd/survey.py                 sizes and memory estimate for every interface texture
python tools/ui_hd/generate.py               core windows: 4x masters and the 2x tier
python tools/ui_hd/generate.py --set rest    every other texture: 2x tier only
python tools/ui_hd/generate.py --only newui_item_back01.OZT
python tools/ui_hd/verify.py                 reduce every HD texture back and compare it with its original
python tools/ui_hd/compare.py <original> <hd.png> --sheet out.png    one texture, with a side-by-side picture
python tools/ui_hd/manifest.py               rewrite tools/ui_hd/manifest.csv
```

To add a texture to the core set, add its path (relative to `Data/Interface`) to a group in
`tools/ui_hd/core_set.py` and run `generate.py`. To hand-paint or redraw a texture, replace its `Interface@4x` PNG with
your own (exactly 4x the original size, content at the same position), then regenerate the 2x tier with
`python tools/ui_hd/tiers.py` (do not run `generate.py` afterwards, it would overwrite your master).

`tools/ui_hd/manifest.csv` lists all interface textures and whether they have HD art.

Two textures that differ only by extension (for example `newui_btn_empty.OZJ` and `newui_btn_empty.OZT`) would share one
HD path, so neither gets HD art.

## How the art is made

The enlargement uses Real-ESRGAN (model `realesr-animevideov3`, 4x) on the colour and on the transparency separately.
The network invents crisp edges and fine detail but shifts contrast and colour, so each result is merged with the plain
enlargement: detail from the network, low-frequency tones from the original. Reduced back to the original size, the
result matches the original (average error about 3 of 255 on the core set).

Every result is checked against its original (`generate.py` does it, `verify.py` repeats it). A texture whose result
drifts too far falls back to a plain Lanczos enlargement; if that drifts too, it gets no HD art. Very small textures
(under 16 pixels on a side) and the stone window background use the plain enlargement (sharpened Lanczos, seeded
fine grain for the stone), because the network does not help there.

## How it works (usage-level)

The game keeps treating each interface bitmap as its original size. The HD texture only changes how many texels back
that rectangle, so every draw call and every UV calculation stays valid. If an HD file is present it replaces the
original; otherwise the original is loaded exactly as before.
