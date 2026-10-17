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

The setting is read at start-up. You can also change it in game: Options, Graphics page, "Texture quality (restart)"
(Auto, Ultra, High, Standard). Restart the game after changing it. A missing HD file is never an error: that one
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

Needs Python 3 with Pillow and numpy, and the Real-ESRGAN program (a small standalone program that enlarges images; it
is not part of the game or of the repository, and it is only needed to regenerate art, never to play).

One-time setup (Windows, needs a Vulkan-capable GPU):

```
pip install pillow numpy
python tools/ui_hd/setup_esrgan.py
```

`setup_esrgan.py` downloads the official release
`realesrgan-ncnn-vulkan-20220424-windows.zip` (about 45 MB) from
`https://github.com/xinntao/Real-ESRGAN/releases/tag/v0.2.5.0`, checks its SHA-256
(`abc02804e17982a3be33675e4d471e91ea374e65b70167abc09e31acb412802d`) and unpacks `realesrgan-ncnn-vulkan.exe`,
`vcomp140.dll` and the `models` folder into `tools/ui_hd/.esrgan/` (git-ignored). To use another copy, set the
`UI_HD_ESRGAN` environment variable to the path of `realesrgan-ncnn-vulkan.exe` (its `models` folder must be next to it).
Without the program, `generate.py --method lanczos` still works but looks softer.

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

## Redrawn textures

A few textures are not enlargements but are redrawn by `generate.py` at 4x, because enlarging them keeps their
problems (pixel-font captions, notched corners, noisy shading):

| Texture | What is redrawn | Module |
|---|---|---|
| Create, Connect, Delete, Menu, Credit buttons (login and character select) | round-cornered button with a caption set in DejaVu Sans Bold | `retext.py` |
| Hotkey slots (Q W E R, 2 to 5), potion slot, the five bottom-right menu buttons, skill boxes | round frame; the original face and icon are kept | `reframe.py` |
| Outer top corners of the bottom bar | rounded, steel edge line instead of gold | `reframe.py` |
| AG and SD tubes | smooth cylinder shading | `gauges.py` |

The skill boxes and menu buttons have transparent corners, so the game turns alpha blending on before drawing them.

The stone window backdrop (`newui_msgbox_back`) is drawn with round corners by every window that draws it through
`SEASON3B::RenderImage`; the bottom corners of `newui_item_back03` are rounded in the art to match.

## Bottom HUD layout

The hotkey slots, the orb bar and the menu buttons are one centred group (the original 640-wide layout) at any
window size; only the experience bar spans the whole window. Earlier builds spread the three parts to the window edges.

## Choosing the model

`generate.py` uses `realesrgan-x4plus` through `--model`, or the `UI_HD_MODEL` environment variable. A texture that
drifts too far from its original falls back to `realesr-animevideov3`, then to plain enlargement, then to no HD art.
`generate.py --missing` only builds textures that have no HD file yet.

## How it works (usage-level)

The game keeps treating each interface bitmap as its original size. The HD texture only changes how many texels back
that rectangle, so every draw call and every UV calculation stays valid. If an HD file is present it replaces the
original; otherwise the original is loaded exactly as before.

## Models, terrain and scenery

The same mechanism covers more than the interface. HD art is looked up for the folders `Interface`, `Skill`, `Item`,
`Player`, `Monster`, `NPC` and the numbered map folders `World1`, `World2`, ... and `Object1`, `Object2`, ... (the terrain
tiles and the scenery of each map). Everything else (effects, sprites) keeps its original art.

| Folder | Tiers present |
|---|---|
| `Player`, `Monster`, `NPC` | 2x for every texture; 4x for the largest ones (all of `Player`, 260 `Monster`, 120 `NPC`) |
| `Item`, `Skill` | 2x for every texture (wings, weapons, armour, mounts) |
| `WorldN`, `ObjectN` | 2x for every map; terrain and scenery tiles are enlarged with wrap padding so they stay seamless |

Details that matter when regenerating or adding model art:

- **Same name, different picture.** An original can exist twice with different pictures (`Foo.OZJ` and `Foo.OZT`). Such
  textures keep the extension in the HD name (`Foo.jpg.png`, `Foo.tga.png`); the game tries that name first and falls back to
  `Foo.png`. Do not rename them.
- **JPEG versus TGA.** A texture that came from a JPEG must stay opaque and be reported as 3 components, a TGA as 4;
  the renderer chooses additive or alpha blending from that. HD textures report the component count of the original.
- **Cut-outs.** Some cut-out textures (the Dark Lord cape cloth, for example) are drawn without an alpha test, so the colour
  under fully transparent pixels is visible. Those are enlarged without premultiplying (`fix_cutouts.py` regenerates all of
  them), which keeps that colour instead of turning it black.
- **Small textures.** Textures of 32 pixels or less (potions, jewels, fruit) are left at their original look.
- **Unreadable originals.** A few 24-bit `.OZT` files cannot be read by the game either; they are skipped.

Tools: `model_textures.py` (mounts and selected models), `fix_cutouts.py` (cut-out textures), `generate.py` and
`verify.py` as above. Run them in small batches (`UI_HD_BATCH`), the enlargement needs a lot of memory.

## Texture quality in game

Options, Graphics page, **Texture quality (restart)** chooses what to load:

| Choice | `HdTextures` | Meaning |
|---|---|---|
| Auto | `auto` | the best art that exists (the 4x art, else 2x, else the original) |
| Ultra | `4` | same as Auto |
| High | `2` | only the 2x art, lighter on memory |
| Standard | `off` | the original textures |

The choice is written to `config.ini` at once and read at start-up, so it takes effect after a restart. A texture picks
its file once, when it is loaded.

## Texture compression

The large HD textures of characters, monsters, NPCs, items and skills are block-compressed when they are sent to
the graphics card: BC1 (half a byte per pixel) for opaque art, BC3 (one byte per pixel, with alpha) for art with
transparency. Compared with uncompressed RGBA (four bytes per pixel) that is a quarter to an eighth of the video memory.
The mip levels are built on the CPU, because compressed textures cannot be mipmapped on the graphics card, and the CPU
copy of the picture is freed afterwards. On the login screen this took the video memory from 1.8 GB to 1.1 GB and the
RAM from 1.2 GB to 0.6 GB.

The interface art stays uncompressed (thin lines and flat colours show the artefacts), and so do the terrain and
scenery tiles: compressing them made the tall grass in the Lorencia flower beds disappear. A graphics card that cannot
sample the format simply gets the uncompressed texture. To switch compression off:

```ini
[UI]
HdCompression=0
```

The encoder is `src/source/Render/Textures/BlockCompression.h` (a small endpoint-fitting encoder, tested by
`tests/render/test_block_compression.cpp`).
