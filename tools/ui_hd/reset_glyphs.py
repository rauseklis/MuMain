"""Draws the glyphs of the reset NPC window: the level arrow, the check, the cross and the question mark button.

  python tools/ui_hd/reset_glyphs.py

One sheet of four 32 x 32 cells (level, check, cross, help), drawn at 8x and reduced, so every edge is smooth. Writes
src/bin/Data/Interface/newui_reset_glyphs.OZT and its Interface@4x and Interface@2x tiers. The check and the cross are white
(the window tints them green and red); the level arrow and the help button carry their own gold.
"""
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

import generate
from mu_image import write_ozt

RELATIVE = "newui_reset_glyphs.OZT"
CELL = 32
CELLS = 4
SUPER = 8
FONT_CANDIDATES = ("georgiab.ttf", "timesbd.ttf", "arialbd.ttf")


def gradient(size, top, bottom):
    ramp = np.linspace(0.0, 1.0, size[1])[:, None, None]
    top = np.array(top, dtype=np.float32)[None, None, :]
    bottom = np.array(bottom, dtype=np.float32)[None, None, :]
    column = top + (bottom - top) * ramp
    return Image.fromarray(np.repeat(column, size[0], axis=1).astype(np.uint8), "RGBA")


def stroke(draw, points, width, fill=255):
    """A smooth polyline with round joins and caps."""
    radius = width / 2.0
    for (x1, y1), (x2, y2) in zip(points, points[1:]):
        steps = max(2, int(math.hypot(x2 - x1, y2 - y1) * 2))
        for step in range(steps + 1):
            t = step / steps
            x = x1 + (x2 - x1) * t
            y = y1 + (y2 - y1) * t
            draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=fill)


def cell_canvas():
    size = CELL * SUPER
    return Image.new("L", (size, size), 0)


def check_cell():
    mask = cell_canvas()
    s = SUPER
    stroke(ImageDraw.Draw(mask), [(7 * s, 17 * s), (13.5 * s, 23.5 * s), (25 * s, 8.5 * s)], 4.4 * s)
    cell = Image.new("RGBA", mask.size, (255, 255, 255, 0))
    cell.putalpha(mask)
    return cell


def cross_cell():
    mask = cell_canvas()
    s = SUPER
    draw = ImageDraw.Draw(mask)
    stroke(draw, [(8.5 * s, 8.5 * s), (23.5 * s, 23.5 * s)], 4.4 * s)
    stroke(draw, [(23.5 * s, 8.5 * s), (8.5 * s, 23.5 * s)], 4.4 * s)
    cell = Image.new("RGBA", mask.size, (255, 255, 255, 0))
    cell.putalpha(mask)
    return cell


def outlined(mask, top, bottom, rim=(26, 18, 8, 255), grow=1.4):
    """Fills the mask with a vertical gradient and surrounds it with a dark rim."""
    size = mask.size
    cell = Image.new("RGBA", size, (0, 0, 0, 0))
    outer = mask.filter(ImageFilter.MaxFilter(int(grow * SUPER) * 2 + 1))
    cell.paste(Image.new("RGBA", size, rim), (0, 0), outer)
    cell.paste(gradient(size, top, bottom), (0, 0), mask)
    return cell


def level_cell():
    """A bold arrow pointing up above a short bar: the level going up."""
    mask = cell_canvas()
    s = SUPER
    draw = ImageDraw.Draw(mask)
    draw.polygon([(16 * s, 4.5 * s), (26.5 * s, 16 * s), (19.6 * s, 16 * s), (19.6 * s, 22 * s),
                  (12.4 * s, 22 * s), (12.4 * s, 16 * s), (5.5 * s, 16 * s)], fill=255)
    draw.rounded_rectangle((8.5 * s, 25 * s, 23.5 * s, 28 * s), radius=1.5 * s, fill=255)
    cell = outlined(mask, (255, 240, 176, 255), (214, 152, 44, 255))
    # a soft highlight along the upper left edge of the arrow
    shine = Image.new("L", mask.size, 0)
    ImageDraw.Draw(shine).polygon([(16 * s, 6.4 * s), (7.6 * s, 15 * s), (11 * s, 15 * s), (16 * s, 9.6 * s)], fill=120)
    shine = shine.filter(ImageFilter.GaussianBlur(0.8 * s))
    highlight = Image.new("RGBA", mask.size, (255, 255, 255, 0))
    highlight.putalpha(Image.fromarray(np.minimum(np.array(shine), np.array(mask)), "L"))
    return Image.alpha_composite(cell, highlight)


def help_cell():
    """A round gold-rimmed button with a question mark."""
    size = CELL * SUPER
    s = SUPER
    centre = size / 2.0
    cell = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    def disc(radius, top, bottom):
        mask = Image.new("L", (size, size), 0)
        ImageDraw.Draw(mask).ellipse((centre - radius, centre - radius, centre + radius, centre + radius), fill=255)
        cell.paste(gradient((size, size), top, bottom), (0, 0), mask)

    disc(15.4 * s, (22, 14, 4, 255), (22, 14, 4, 255))
    disc(14.6 * s, (246, 214, 124, 255), (170, 122, 40, 255))
    disc(12.6 * s, (52, 42, 24, 255), (16, 12, 7, 255))

    font = None
    for name in FONT_CANDIDATES:
        try:
            font = ImageFont.truetype(name, int(21 * s))
            break
        except OSError:
            continue
    if font is None:
        raise SystemExit("no bold font found")
    text = Image.new("L", (size, size), 0)
    ImageDraw.Draw(text).text((centre, centre + 0.5 * s), "?", font=font, anchor="mm", fill=255)
    cell.paste(gradient((size, size), (255, 236, 160, 255), (226, 170, 60, 255)), (0, 0), text)
    return cell


def main():
    sheet = Image.new("RGBA", (CELL * CELLS * SUPER, CELL * SUPER), (0, 0, 0, 0))
    for index, cell in enumerate((level_cell(), check_cell(), cross_cell(), help_cell())):
        sheet.paste(cell, (index * CELL * SUPER, 0))

    master = np.array(sheet.resize((CELL * CELLS * 4, CELL * 4), Image.LANCZOS))
    base = np.array(sheet.resize((CELL * CELLS, CELL), Image.LANCZOS))
    write_ozt(generate.source_path(RELATIVE), base)
    generate.write_tiers(RELATIVE, master, (4, 2))
    print("ok", master.shape)


if __name__ == "__main__":
    main()
