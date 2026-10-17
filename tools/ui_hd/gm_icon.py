"""Draws the missing Game Master buff icon (buff 28) into the buff icon atlas at 4x.

The original atlas has a black placeholder showing the buff number ("28") in that cell. The icon is drawn from
scratch: a deep violet field with a gold glow, a gold shield with a crown and the letters GM, and a thin steel border
like the other buff icons. Cells are 20 x 28 logical pixels, ten to a row.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

SCALE = 4
SUPERSAMPLE = 3
CELL_WIDTH = 20
CELL_HEIGHT = 28
CELLS_PER_ROW = 10
GM_BUFF_NUMBER = 28
ATLAS = "newui_statusicon.ozj"
FONT_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "src", "bin", "fonts", "DejaVuSans-Bold.ttf")

FIELD_TOP = (46, 34, 108)
FIELD_BOTTOM = (18, 12, 52)
GLOW = (255, 200, 80)
GOLD_TOP = (255, 236, 150)
GOLD_BOTTOM = (214, 142, 24)
GOLD_EDGE = (92, 52, 8)
LETTER = (40, 22, 70)
BORDER_LIGHT = (170, 172, 182)
BORDER_DARK = (70, 72, 82)


def cell_origin(buff_number):
    index = buff_number - 1
    return (index % CELLS_PER_ROW) * CELL_WIDTH * SCALE, (index // CELLS_PER_ROW) * CELL_HEIGHT * SCALE


def vertical_gradient(size, top, bottom):
    rows = np.linspace(0.0, 1.0, size[1], dtype=np.float32)[:, None, None]
    colours = np.array(top, np.float32) * (1 - rows) + np.array(bottom, np.float32) * rows
    return Image.fromarray(np.clip(np.broadcast_to(colours, (size[1], size[0], 3)), 0, 255).astype(np.uint8), "RGB")


def shield_points(width, height):
    """Shield outline in a width x height box: flat top with a notch, straight sides, curved point at the bottom."""
    left, right = width * 0.20, width * 0.80
    top, shoulder, tip = height * 0.34, height * 0.40, height * 0.90
    points = [(left, top), (width * 0.5, shoulder - height * 0.03), (right, top), (right, height * 0.62)]
    for step in range(1, 13):
        t = step / 12.0
        x = right - (right - width * 0.5) * t
        y = height * 0.62 + (tip - height * 0.62) * (t ** 1.7)
        points.append((x, y))
    for step in range(1, 13):
        t = step / 12.0
        x = width * 0.5 - (width * 0.5 - left) * t
        y = tip - (tip - height * 0.62) * (t ** (1 / 1.7))
        points.append((x, y))
    return points


def crown_points(width, height):
    base, tip = height * 0.30, height * 0.12
    left, right, mid = width * 0.28, width * 0.72, width * 0.5
    return [(left, base), (left, tip + height * 0.05), (width * 0.38, base - height * 0.07), (mid, tip),
            (width * 0.62, base - height * 0.07), (right, tip + height * 0.05), (right, base)]


def draw_gm_icon():
    big = (CELL_WIDTH * SCALE * SUPERSAMPLE, CELL_HEIGHT * SCALE * SUPERSAMPLE)
    image = vertical_gradient(big, FIELD_TOP, FIELD_BOTTOM).convert("RGBA")

    glow = Image.new("L", big, 0)
    ImageDraw.Draw(glow).ellipse([big[0] * 0.1, big[1] * 0.25, big[0] * 0.9, big[1] * 0.85], fill=170)
    glow = glow.filter(ImageFilter.GaussianBlur(big[0] * 0.14))
    image.alpha_composite(Image.merge("RGBA", [Image.new("L", big, c) for c in GLOW] + [glow]))

    draw = ImageDraw.Draw(image)
    outline = max(big[0] // 40, 2)
    draw.polygon(shield_points(*big), fill=GOLD_EDGE)
    for points in (shield_points(*big),):
        mask = Image.new("L", big, 0)
        ImageDraw.Draw(mask).polygon(points, fill=255)
        inner = mask.filter(ImageFilter.MinFilter(outline * 2 + 1))
        gold = vertical_gradient(big, GOLD_TOP, GOLD_BOTTOM).convert("RGBA")
        gold.putalpha(inner)
        image.alpha_composite(gold)
    draw = ImageDraw.Draw(image)
    crown_mask = Image.new("L", big, 0)
    ImageDraw.Draw(crown_mask).polygon(crown_points(*big), fill=255)
    crown_inner = crown_mask.filter(ImageFilter.MinFilter(outline * 2 + 1))
    draw.polygon(crown_points(*big), fill=GOLD_EDGE)
    crown = vertical_gradient(big, GOLD_TOP, GOLD_BOTTOM).convert("RGBA")
    crown.putalpha(crown_inner)
    image.alpha_composite(crown)

    font = ImageFont.truetype(FONT_PATH, int(big[1] * 0.20))
    left, top, right, bottom = font.getbbox("GM")
    origin = ((big[0] - (right - left)) / 2 - left, big[1] * 0.50 - (bottom - top) / 2 - top)
    draw = ImageDraw.Draw(image)
    draw.text(origin, "GM", font=font, fill=LETTER)

    border = outline * 2
    frame = Image.new("RGBA", big, (0, 0, 0, 0))
    fd = ImageDraw.Draw(frame)
    fd.rectangle([0, 0, big[0] - 1, big[1] - 1], outline=BORDER_DARK + (255,), width=border)
    fd.rectangle([border, border, big[0] - 1 - border, big[1] - 1 - border], outline=BORDER_LIGHT + (255,), width=outline)
    image.alpha_composite(frame)
    return np.asarray(image.resize((CELL_WIDTH * SCALE, CELL_HEIGHT * SCALE), Image.LANCZOS).convert("RGBA"))


def apply(name, master):
    if name.lower() != ATLAS:
        return master
    out = master.copy()
    x, y = cell_origin(GM_BUFF_NUMBER)
    icon = draw_gm_icon()
    out[y:y + icon.shape[0], x:x + icon.shape[1]] = icon
    return out
