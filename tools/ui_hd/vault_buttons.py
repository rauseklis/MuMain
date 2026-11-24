"""Draws the previous/next vault buttons of the storage window (24 x 29 logical pixels, two states stacked).

  python tools/ui_hd/vault_buttons.py

Writes, for newui_vault_btn_L and newui_vault_btn_R:
  src/bin/Data/Interface/<name>.OZT      the original-size texture the game loads (24 x 58: up, pressed)
  src/bin/Data/Interface@2x/<name>.png   the 2x tier (48 x 116)
  src/bin/Data/Interface@4x/<name>.png   the 4x master (96 x 232)

The frame is the real frame of the round Zen / lock buttons beside them (newui_Bt_lock, 2x tier): the padlock is wiped
off the plate, the frame is cut into a nine-patch and made narrower, and a gold arrow with the same dark outline as the
"+" of the expand button is drawn on it. The second state is the dimmed frame of the same texture, like the other buttons.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

from upscale import gaussian_blur

WIDTH, HEIGHT = 24, 29
SCALE = 4
SUPERSAMPLE = 3
NAMES = {"L": "newui_vault_btn_L", "R": "newui_vault_btn_R"}
SOURCE = "newui_Bt_lock"

EDGE_PATCH = 11  # logical pixels of the source frame kept as they are on each side (the bevel with the notches)
PLATE_INSET = 5.0  # logical pixels from the outside edge to the flat plate
GOLD_TOP = (255, 236, 150)
GOLD_BOTTOM = (214, 128, 20)
GLYPH_OUTLINE = (30, 14, 4)
DIM = 0.62  # the pressed state shows the glyph dimmed, like the lock button does


def data_root():
    return os.path.join(os.path.dirname(__file__), "..", "..", "src", "bin", "Data")


def load_states():
    """Returns the two 4x states (up, pressed) of the source button as float RGBA arrays."""
    sheet = Image.open(os.path.join(data_root(), "Interface@2x", SOURCE + ".png")).convert("RGBA")
    half = sheet.height // 2
    states = []
    for index in range(2):
        state = sheet.crop((0, index * half, sheet.width, (index + 1) * half))
        states.append(np.asarray(state.resize((state.width * 2, state.height * 2), Image.LANCZOS), np.float32))
    return states


def wipe_plate(state):
    """Replaces the glyph in the middle of the plate by a plate of the same metal (rows copied from the clear sides)."""
    height, width = state.shape[:2]
    inset = int(PLATE_INSET * SCALE)
    clear = int(2.4 * SCALE)  # width of the strip beside the glyph that is free of it
    left = state[:, inset : inset + clear, :].mean(axis=1)
    right = state[:, width - inset - clear : width - inset, :].mean(axis=1)
    columns = np.linspace(0.0, 1.0, width - 2 * inset, dtype=np.float32)[None, :, None]
    plate = left[:, None, :] * (1 - columns) + right[:, None, :] * columns
    rng = np.random.default_rng(11)
    grain = gaussian_blur(rng.normal(0.0, 1.0, (height, width - 2 * inset)).astype(np.float32), 0.9)
    grain *= 3.2 / max(float(grain.std()), 1e-6)
    plate[..., :3] += grain[..., None]
    wiped = state.copy()
    wiped[inset : height - inset, inset : width - inset, :] = plate[inset : height - inset, :, :]
    return wiped


def nine_patch_narrow(state, new_width):
    """Keeps the bevelled left and right edges and shrinks the flat middle of the frame."""
    height, width = state.shape[:2]
    edge = EDGE_PATCH * SCALE
    left = state[:, :edge, :]
    right = state[:, width - edge :, :]
    middle = Image.fromarray(np.clip(state[:, edge : width - edge, :], 0, 255).astype(np.uint8), "RGBA")
    middle = np.asarray(middle.resize((new_width - 2 * edge, height), Image.LANCZOS), np.float32)
    return np.concatenate([left, middle, right], axis=1)


def vertical_gradient(size, top, bottom):
    rows = np.linspace(0.0, 1.0, size[1], dtype=np.float32)[:, None, None]
    colours = np.array(top, np.float32) * (1 - rows) + np.array(bottom, np.float32) * rows
    return np.broadcast_to(colours, (size[1], size[0], 3)).copy()


def big(value):
    return int(round(value * SCALE * SUPERSAMPLE))


def paste_layer(canvas, colours, mask):
    layer = Image.fromarray(np.clip(colours, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    layer.putalpha(mask)
    canvas.alpha_composite(layer)


def chevron_points(cx, cy, half_height, depth, thickness, pointing_left):
    """A bold chevron with a notch at the back, like the arrowheads of the quest window, as one polygon."""
    sign = -1.0 if pointing_left else 1.0
    tip = (cx + sign * depth / 2.0, cy)
    top = (cx - sign * depth / 2.0, cy - half_height)
    bottom = (cx - sign * depth / 2.0, cy + half_height)
    notch = (cx - sign * (depth / 2.0 - thickness), cy)
    return [tip, top, notch, bottom]


def draw_glyph(size, side, dim):
    layer = Image.new("RGBA", size, (0, 0, 0, 0))
    cx = WIDTH / 2.0 + (-0.2 if side == "L" else 0.2)
    cy = HEIGHT / 2.0 + 0.1
    outer = [(big(x), big(y)) for x, y in chevron_points(cx, cy, 7.4, 9.4, 3.0, side == "L")]
    inner = [(big(x), big(y)) for x, y in chevron_points(cx + (0.5 if side == "L" else -0.5), cy, 5.6, 7.0, 2.3, side == "L")]

    outline_mask = Image.new("L", size, 0)
    ImageDraw.Draw(outline_mask).polygon(outer, fill=255)
    outline_mask = outline_mask.filter(ImageFilter.GaussianBlur(big(0.1)))
    outline = np.full((size[1], size[0], 3), GLYPH_OUTLINE, np.float32)
    paste_layer(layer, outline, outline_mask)

    glyph_mask = Image.new("L", size, 0)
    ImageDraw.Draw(glyph_mask).polygon(inner, fill=255)
    glyph_mask = glyph_mask.filter(ImageFilter.GaussianBlur(big(0.08)))
    gold = vertical_gradient(size, tuple(c * dim for c in GOLD_TOP), tuple(c * dim for c in GOLD_BOTTOM))
    paste_layer(layer, gold, glyph_mask)

    shine = Image.new("L", size, 0)
    ImageDraw.Draw(shine).line([inner[1], inner[0]], fill=190, width=max(big(0.4), 1))
    shine = shine.filter(ImageFilter.GaussianBlur(big(0.08)))
    paste_layer(layer, np.full((size[1], size[0], 3), (255, 250, 220), np.float32) * dim, shine)
    return layer


def draw_button(side):
    ups = []
    for index, state in enumerate(load_states()):
        frame = nine_patch_narrow(wipe_plate(state), WIDTH * SCALE)
        canvas = Image.fromarray(np.clip(frame, 0, 255).astype(np.uint8), "RGBA")
        size = (WIDTH * SCALE * SUPERSAMPLE, HEIGHT * SCALE * SUPERSAMPLE)
        canvas = canvas.resize(size, Image.BICUBIC)
        canvas.alpha_composite(draw_glyph(size, side, 1.0 if index == 0 else DIM))
        ups.append(canvas.resize((WIDTH * SCALE, HEIGHT * SCALE), Image.LANCZOS))
    sheet = Image.new("RGBA", (WIDTH * SCALE, HEIGHT * SCALE * 2), (0, 0, 0, 0))
    sheet.paste(ups[0], (0, 0))
    sheet.paste(ups[1], (0, HEIGHT * SCALE))
    return sheet


def main():
    root = data_root()
    for side, name in NAMES.items():
        master = draw_button(side)
        from mu_image import write_ozt

        write_ozt(os.path.join(root, "Interface", name + ".OZT"), np.asarray(master.resize((WIDTH, HEIGHT * 2), Image.LANCZOS)))
        master.save(os.path.join(root, "Interface@4x", name + ".png"), optimize=True)
        master.resize((WIDTH * 2, HEIGHT * 4), Image.LANCZOS).save(os.path.join(root, "Interface@2x", name + ".png"), optimize=True)
        print("wrote", name)


if __name__ == "__main__":
    main()
