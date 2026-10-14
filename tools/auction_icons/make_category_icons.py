"""Generates the Auction House category rail icons.

Eight white, duotone glyphs (solid shapes plus softer secondary shapes and cut-out detail) drawn on a 64-unit grid at
8x supersampling and reduced to 32 x 32 cells, one per rail category in rail order. The game multiplies them by the row's
tint (steel when idle, gold when selected), so the artwork itself is neutral.

Output (relative to the client repository root):
  src/bin/Data/Interface/newui_auction_categories.OZT   256 x 32 atlas, cell n starts at x = 32 * n
  tools/auction_icons/categories_preview.png           the atlas tinted idle/selected on the rail's colours

Needs Pillow:  python tools/auction_icons/make_category_icons.py
"""
import math
import os
import struct
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

GRID = 64
SS = 8
BIG = GRID * SS
CELL = 32
REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def blank():
    return Image.new("L", (BIG, BIG), 0)


def pts(points):
    return [(x * SS, y * SS) for x, y in points]


def poly(mask, points, fill=255):
    ImageDraw.Draw(mask).polygon(pts(points), fill=fill)


def rrect(mask, x0, y0, x1, y1, radius, fill=255):
    ImageDraw.Draw(mask).rounded_rectangle((x0 * SS, y0 * SS, x1 * SS, y1 * SS), radius=radius * SS, fill=fill)


def ellipse(mask, x0, y0, x1, y1, fill=255):
    ImageDraw.Draw(mask).ellipse((x0 * SS, y0 * SS, x1 * SS, y1 * SS), fill=fill)


def circle(mask, cx, cy, r, fill=255):
    ellipse(mask, cx - r, cy - r, cx + r, cy + r, fill)


def line(mask, p, q, width, fill=255):
    d = ImageDraw.Draw(mask)
    d.line([(p[0] * SS, p[1] * SS), (q[0] * SS, q[1] * SS)], fill=fill, width=int(width * SS))
    circle(mask, p[0], p[1], width / 2, fill)
    circle(mask, q[0], q[1], width / 2, fill)


def dilate(mask, units):
    size = int(units * SS) * 2 + 1
    return mask.filter(ImageFilter.MaxFilter(size if size % 2 else size + 1))


def erase(mask, region):
    return ImageChops.multiply(mask, ImageChops.invert(region))


class Glyph:
    """Primary shapes (full alpha), detail shapes (about half alpha) and cuts (subtract)."""

    def __init__(self):
        self.p = blank()
        self.d = blank()
        self.c = blank()

    def render(self):
        detail = self.d.point(lambda v: int(v * 0.5))
        body = ImageChops.lighter(self.p, detail)
        return ImageChops.multiply(body, ImageChops.invert(self.c))


def sword():
    g = Glyph()
    blade = blank()
    poly(blade, [(32, 2), (36.8, 9), (36.8, 38), (27.2, 38), (27.2, 9)])
    guard = blank()
    rrect(guard, 17.5, 38, 46.5, 43.5, 2.4)
    grip = blank()
    rrect(grip, 29, 43, 35, 54.5, 1.6)
    pommel = blank()
    circle(pommel, 32, 57.5, 4.4)
    cuts = blank()
    rrect(cuts, 31, 9, 33, 34, 1.0)
    line(cuts, (29.4, 46.6), (34.6, 46.6), 1.0)
    line(cuts, (29.4, 50.0), (34.6, 50.0), 1.0)
    for mask, target in ((blade, g.p), (guard, g.p), (grip, g.p), (pommel, g.p), (cuts, g.c)):
        rotated = mask.rotate(-45, resample=Image.BICUBIC, center=(32 * SS, 31 * SS))
        target.paste(ImageChops.lighter(target, rotated))
    return g


def helm():
    g = Glyph()
    ellipse(g.p, 11, 4, 53, 48)
    poly(g.p, [(11, 26), (53, 26), (53, 50), (46, 57.5), (18, 57.5), (11, 50)])
    rrect(g.c, 16.5, 28, 47.5, 34.5, 3.2)
    for x in (23.5, 30.5, 37.5):
        rrect(g.c, x, 40, x + 3.2, 52, 1.5)
    rrect(g.c, 30.2, 6, 33.8, 26, 1.8, fill=95)
    return g


def bezier(p0, p1, p2, steps=32):
    out = []
    for i in range(steps + 1):
        u = i / steps
        out.append(((1 - u) ** 2 * p0[0] + 2 * (1 - u) * u * p1[0] + u * u * p2[0],
                    (1 - u) ** 2 * p0[1] + 2 * (1 - u) * u * p1[1] + u * u * p2[1]))
    return out


def wing():
    g = Glyph()
    base, tip, control = (6.0, 52.0), (56.0, 6.0), (3.0, 12.0)
    chord = (base[0] - tip[0], base[1] - tip[1])
    length = math.hypot(*chord)
    normal = (-chord[1] / length, chord[0] / length)
    if normal[0] < 0:
        normal = (-normal[0], -normal[1])
    feathers = 5
    reach = (8.0, 12.5, 14.0, 12.0, 7.5)
    outline = bezier(base, control, tip)
    cuts = []
    for i in range(feathers):
        tip_t = (i + 0.62) / feathers
        notch_t = (i + 1.0) / feathers
        feather_tip = (tip[0] + chord[0] * tip_t + normal[0] * reach[i], tip[1] + chord[1] * tip_t + normal[1] * reach[i])
        notch = (tip[0] + chord[0] * notch_t + normal[0] * 3.0, tip[1] + chord[1] * notch_t + normal[1] * 3.0)
        outline += [feather_tip, notch]
        if i < feathers - 1:
            toward_tip = (-chord[0] / length, -chord[1] / length)
            direction = (-normal[0] + 0.45 * toward_tip[0], -normal[1] + 0.45 * toward_tip[1])
            size = math.hypot(*direction)
            direction = (direction[0] / size, direction[1] / size)
            cuts.append((notch, (notch[0] + direction[0] * 21, notch[1] + direction[1] * 21)))
    poly(g.p, outline)
    circle(g.p, base[0] + 1.5, base[1] - 1.5, 4.6)
    for a, b in cuts:
        line(g.c, a, b, 1.5)
    return g


def paw():
    g = Glyph()
    ellipse(g.p, 6.5, 25, 19.5, 42)
    ellipse(g.p, 17, 9, 31, 29)
    ellipse(g.p, 33, 9, 47, 29)
    ellipse(g.p, 44.5, 25, 57.5, 42)
    ellipse(g.p, 15.5, 34, 48.5, 58.5)
    circle(g.p, 23, 41.5, 7.8)
    circle(g.p, 41, 41.5, 7.8)
    rrect(g.c, 24.5, 38.5, 27.2, 46, 1.3, fill=90)
    return g


def gem():
    g = Glyph()
    poly(g.p, [(20, 7), (44, 7), (58.5, 24), (32, 58.5), (5.5, 24)])
    line(g.c, (5.5, 24), (58.5, 24), 1.5)
    for a, b in (((20, 7), (26.5, 24)), ((44, 7), (37.5, 24)), ((32, 7), (26.5, 24)), ((32, 7), (37.5, 24)),
                 ((26.5, 24), (32, 58.5)), ((37.5, 24), (32, 58.5))):
        line(g.c, a, b, 1.5)
    poly(g.c, [(5.5, 24), (20, 7), (26.5, 24)], fill=70)
    poly(g.c, [(58.5, 24), (44, 7), (37.5, 24)], fill=70)
    return g


def potion():
    g = Glyph()
    flask = blank()
    rrect(flask, 26, 12, 38, 32, 1.5)
    circle(flask, 32, 41.5, 17.5)
    level = 36 * SS
    liquid = flask.copy()
    ImageDraw.Draw(liquid).rectangle((0, 0, BIG, level), fill=0)
    glass = flask.copy()
    ImageDraw.Draw(glass).rectangle((0, level, BIG, BIG), fill=0)
    g.p = ImageChops.lighter(g.p, liquid)
    g.d = ImageChops.lighter(g.d, glass)
    rrect(g.p, 23.5, 3, 40.5, 11, 2.6)
    for cx, cy, r in ((26.5, 47.5, 2.3), (36.5, 50.5, 2.0), (34.5, 43.5, 1.6)):
        circle(g.c, cx, cy, r)
    rrect(g.c, 22.2, 35.5, 24.8, 47, 1.3, fill=140)
    return g


def scroll():
    g = Glyph()
    rrect(g.d, 14, 17, 50, 47, 1.2)
    for y in (24, 30, 36):
        line(g.p, (20, y), (44 if y != 36 else 36, y), 2.2)
    rrect(g.p, 8.5, 7.5, 55.5, 19, 5.0)
    rrect(g.p, 8.5, 45, 55.5, 56.5, 5.0)
    for cx, cy in ((13.2, 13.2), (50.8, 13.2), (13.2, 50.8)):
        circle(g.c, cx, cy, 2.5)
    ring = blank()
    circle(ring, 46, 42, 11.8)
    g.p = erase(g.p, ring)
    g.d = erase(g.d, ring)
    circle(g.p, 46, 42, 9.6)
    circle(g.c, 42.6, 38.4, 2.4, fill=170)
    return g


def chest():
    g = Glyph()
    rrect(g.p, 6.5, 28, 57.5, 57, 3.5)
    lid = blank()
    ellipse(lid, 6.5, 8, 57.5, 47)
    ImageDraw.Draw(lid).rectangle((0, 28 * SS, BIG, BIG), fill=0)
    g.p = ImageChops.lighter(g.p, lid)
    line(g.c, (6.5, 28.3), (57.5, 28.3), 1.5)
    rrect(g.c, 16.5, 10, 20.5, 57, 0.8, fill=95)
    rrect(g.c, 43.5, 10, 47.5, 57, 0.8, fill=95)
    ring = blank()
    rrect(ring, 24, 21.5, 40, 40.5, 3.4)
    g.p = erase(g.p, ring)
    rrect(g.p, 26.2, 23.7, 37.8, 38.3, 2.4)
    circle(g.c, 32, 29.6, 2.3)
    rrect(g.c, 31.1, 30.2, 32.9, 35.4, 0.8)
    return g


ICONS = [sword, helm, wing, paw, gem, potion, scroll, chest]


def cell(glyph):
    alpha = glyph.render().resize((CELL, CELL), Image.LANCZOS)
    rgb = Image.new("RGB", (CELL, CELL))
    px = rgb.load()
    for y in range(CELL):
        shade = int(255 - 45 * (y / (CELL - 1)))
        for x in range(CELL):
            px[x, y] = (shade, shade, shade)
    out = rgb.convert("RGBA")
    out.putalpha(alpha)
    return out


def png_to_ozt(image, destination):
    nx, ny = image.size
    px = image.load()
    header = bytearray()
    header += bytes([0, 0, 2]) + bytes(5) + bytes(2) + bytes(2) + bytes(4)
    header += struct.pack("<h", nx) + struct.pack("<h", ny) + bytes([32, 0])
    assert len(header) == 22
    body = bytearray()
    for y in range(ny - 1, -1, -1):
        for x in range(nx):
            r, g, b, a = px[x, y]
            body += bytes([b, g, r, a])
    footer = bytes(8) + b"TRUEVISION-XFILE.\x00"
    with open(destination, "wb") as f:
        f.write(header)
        f.write(body)
        f.write(footer)


def tinted(atlas, tint):
    layer = Image.new("RGBA", atlas.size, tint + (255,))
    out = ImageChops.multiply(atlas, layer)
    out.putalpha(ImageChops.multiply(atlas.getchannel("A"), layer.getchannel("A")))
    return out


def preview(atlas, destination):
    scale = 6
    row_h, pad = 22, 6
    width = CELL * len(ICONS) // 1
    sheet = Image.new("RGBA", (len(ICONS) * 44 + pad, 2 * (row_h + pad) + pad), (14, 16, 22, 255))
    states = [((190, 196, 208), (24, 28, 36, 255)), ((255, 214, 118), (52, 42, 22, 255))]
    for r, (tint, card) in enumerate(states):
        y = pad + r * (row_h + pad)
        for i in range(len(ICONS)):
            x = pad + i * 44
            ImageDraw.Draw(sheet).rounded_rectangle((x, y, x + 38, y + row_h - 1), radius=4, fill=card)
            icon = tinted(atlas.crop((i * CELL, 0, i * CELL + CELL, CELL)), tint).resize((14, 14), Image.LANCZOS)
            sheet.alpha_composite(icon, (x + 4, y + (row_h - 14) // 2))
    sheet.resize((sheet.width * scale, sheet.height * scale), Image.NEAREST).save(destination)


def main():
    atlas = Image.new("RGBA", (CELL * len(ICONS), CELL), (0, 0, 0, 0))
    for index, make in enumerate(ICONS):
        atlas.alpha_composite(cell(make()), (index * CELL, 0))
    ozt = os.path.join(REPO, "src", "bin", "Data", "Interface", "newui_auction_categories.OZT")
    png_to_ozt(atlas, ozt)
    atlas.save(os.path.join(REPO, "tools", "auction_icons", "categories_atlas.png"))
    preview(atlas, os.path.join(REPO, "tools", "auction_icons", "categories_preview.png"))
    big = Image.new("RGBA", (len(ICONS) * 140, 140), (14, 16, 22, 255))
    for index, make in enumerate(ICONS):
        a = make().render().resize((128, 128), Image.LANCZOS)
        layer = Image.new("RGBA", (128, 128), (205, 210, 222, 255))
        layer.putalpha(a)
        big.alpha_composite(layer, (index * 140 + 6, 6))
    big.save(os.path.join(REPO, "tools", "auction_icons", "categories_big.png"))
    print("wrote", ozt, atlas.size)


if __name__ == "__main__":
    sys.exit(main())
