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

Needs Python 3 with Pillow and numpy.

```
python tools/ui_hd/survey.py                 sizes and memory estimate for every interface texture
python tools/ui_hd/generate.py               enlarge every core texture (4x master, then 2x)
python tools/ui_hd/generate.py --only newui_item_back01.OZT
python tools/ui_hd/compare.py <original> <hd.png> --sheet out.png    fidelity check (MAE, SSIM, side by side)
python tools/ui_hd/manifest.py               rewrite tools/ui_hd/manifest.csv
```

To add a texture, add its path (relative to `Data/Interface`) to the matching group in `tools/ui_hd/core_set.py` and run
`generate.py`. To hand-paint or redraw a texture, replace its `Interface@4x` PNG with your own (exactly 4x the original
size, content at the same position), then regenerate the 2x tier with `python tools/ui_hd/tiers.py` (reduces every
master in `Interface@4x`; do not run `generate.py` afterwards, it would overwrite your master).

`tools/ui_hd/manifest.csv` lists all interface textures and whether they have HD art (`done`) or are still `pending`.

## How the art is made

No AI model is used. The enlargement is a premultiplied-alpha Lanczos upscale, sharpened with an unsharp mask that is
clamped to the neighbourhood of a smooth upscale (so it cannot add halos), with crisper alpha edges. JPEG-sourced
originals are lightly denoised first. The stone window background gets seeded fine grain. This is faithful and
sharper than stretching the original, but it cannot invent detail that the original never had; a hand-redrawn master
will always look better and can be dropped in as described above.

## How it works (usage-level)

The game keeps treating each interface bitmap as its original size. The HD texture only changes how many texels back
that rectangle, so every draw call and every UV calculation stays valid. If an HD file is present it replaces the
original; otherwise the original is loaded exactly as before.
