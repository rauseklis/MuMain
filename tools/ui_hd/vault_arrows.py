"""Draws the previous/next vault arrow buttons of the storage window (20 x 14 logical pixels).

  python tools/ui_hd/vault_arrows.py

Writes, for newui_vault_arrow_L and newui_vault_arrow_R:
  src/bin/Data/Interface/<name>.OZT      the original-size texture the game loads (20 x 14)
  src/bin/Data/Interface@4x/<name>.png   the 4x master (80 x 56)
  src/bin/Data/Interface@2x/<name>.png   the 2x tier (40 x 28)

The look follows the other storage buttons (lock, Zen in/out): a dark steel plate with a silver rim and a gold glyph.
The game dims the button a little while it is idle and draws it at full brightness under the mouse, so the art is full bright.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

from mu_image import write_ozt
from upscale import gaussian_blur

WIDTH, HEIGHT = 20, 14
SCALE = 4
SUPERSAMPLE = 4
NAMES = {"L": "newui_vault_arrow_L", "R": "newui_vault_arrow_R"}

RIM_LIGHT = (214, 214, 220)
RIM_MID = (132, 132, 138)
RIM_DARK = (58, 58, 62)
PLATE_TOP = (62, 62, 66)
PLATE_BOTTOM = (22, 22, 24)
GOLD_TOP = (255, 232, 142)
GOLD_BOTTOM = (206, 122, 22)
GLYPH_OUTLINE = (52, 26, 4)
CORNER = 3.2


def big(value):
    return int(round(value * SCALE * SUPERSAMPLE))


def vertical_gradient(size, top, bottom):
    rows = np.linspace(0.0, 1.0, size[1], dtype=np.float32)[:, None, None]
    colours = np.array(top, np.float32) * (1 - rows) + np.array(bottom, np.float32) * rows
    return np.broadcast_to(colours, (size[1], size[0], 3)).copy()


def rounded_mask(size, inset, radius):
    mask = Image.new("L", size, 0)
    pad = big(inset)
    ImageDraw.Draw(mask).rounded_rectangle([pad, pad, size[0] - 1 - pad, size[1] - 1 - pad], radius=max(big(radius - inset), 2), fill=255)
    return mask


def paste_layer(canvas, colour_array, mask):
    layer = Image.fromarray(np.clip(colour_array, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    layer.putalpha(mask)
    canvas.alpha_composite(layer)


def triangle_points(width, height, cx, cy, pointing_left):
    half_w, half_h = width / 2.0, height / 2.0
    if pointing_left:
        return [(cx - half_w, cy), (cx + half_w, cy - half_h), (cx + half_w, cy + half_h)]
    return [(cx + half_w, cy), (cx - half_w, cy - half_h), (cx - half_w, cy + half_h)]


def draw_button(side):
    size = (big(WIDTH), big(HEIGHT))
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))

    # black outline, silver rim (light at the top left, darker towards the bottom right), dark plate
    paste_layer(canvas, np.zeros((size[1], size[0], 3), np.float32), rounded_mask(size, 0.0, CORNER))
    paste_layer(canvas, vertical_gradient(size, RIM_LIGHT, RIM_DARK), rounded_mask(size, 0.35, CORNER - 0.35))
    paste_layer(canvas, np.zeros((size[1], size[0], 3), np.float32), rounded_mask(size, 1.15, CORNER - 1.15))
    rng = np.random.default_rng(7)
    grain = rng.normal(0.0, 1.0, (size[1], size[0])).astype(np.float32)
    grain = gaussian_blur(grain, big(0.35))
    grain *= 3.0 / max(float(grain.std()), 1e-6)
    plate = vertical_gradient(size, PLATE_TOP, PLATE_BOTTOM) + grain[..., None]
    paste_layer(canvas, plate, rounded_mask(size, 1.55, CORNER - 1.55))

    # a thin lit edge along the top inside the plate, like the other buttons
    edge = Image.new("L", size, 0)
    ImageDraw.Draw(edge).rounded_rectangle([big(1.55), big(1.55), size[0] - 1 - big(1.55), size[1] - 1 - big(1.55)],
                                            radius=max(big(CORNER - 1.55), 2), outline=255, width=max(big(0.28), 1))
    edge_array = np.asarray(edge, np.float32) / 255.0
    top_weight = np.linspace(1.0, 0.0, size[1], dtype=np.float32)[:, None] ** 2
    lit = Image.fromarray((edge_array * top_weight * 120).astype(np.uint8))
    paste_layer(canvas, np.full((size[1], size[0], 3), RIM_MID, np.float32), lit)

    # the glyph: a gold triangle with a dark outline and a highlight along its upper edge
    cx, cy = WIDTH / 2.0 + (-0.35 if side == "L" else 0.35), HEIGHT / 2.0
    outline_points = [(big(x), big(y)) for x, y in triangle_points(8.6, 8.2, cx, cy, side == "L")]
    glyph_points = [(big(x), big(y)) for x, y in triangle_points(6.4, 6.0, cx, cy, side == "L")]
    outline_mask = Image.new("L", size, 0)
    ImageDraw.Draw(outline_mask).polygon(outline_points, fill=255)
    paste_layer(canvas, np.full((size[1], size[0], 3), GLYPH_OUTLINE, np.float32), outline_mask.filter(ImageFilter.GaussianBlur(big(0.12))))
    glyph_mask = Image.new("L", size, 0)
    ImageDraw.Draw(glyph_mask).polygon(glyph_points, fill=255)
    gold = vertical_gradient(size, GOLD_TOP, GOLD_BOTTOM)
    paste_layer(canvas, gold, glyph_mask.filter(ImageFilter.GaussianBlur(big(0.1))))
    shine = Image.new("L", size, 0)
    ImageDraw.Draw(shine).line(glyph_points[0:2] if side == "L" else [glyph_points[0], glyph_points[1]], fill=200, width=max(big(0.45), 1))
    paste_layer(canvas, np.full((size[1], size[0], 3), (255, 250, 214), np.float32), shine.filter(ImageFilter.GaussianBlur(big(0.1))))

    return canvas.resize((WIDTH * SCALE, HEIGHT * SCALE), Image.LANCZOS)


def main():
    data = os.path.join(os.path.dirname(__file__), "..", "..", "src", "bin", "Data")
    for side, name in NAMES.items():
        master = draw_button(side)
        original = master.resize((WIDTH, HEIGHT), Image.LANCZOS)
        write_ozt(os.path.join(data, "Interface", name + ".OZT"), np.asarray(original))
        master.save(os.path.join(data, "Interface@4x", name + ".png"), optimize=True)
        master.resize((WIDTH * 2, HEIGHT * 2), Image.LANCZOS).save(os.path.join(data, "Interface@2x", name + ".png"), optimize=True)
        print("wrote", name)


if __name__ == "__main__":
    main()
