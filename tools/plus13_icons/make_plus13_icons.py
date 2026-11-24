"""Generates the two +13 buff icons: the full-set bonus (golden crest) and the weapon bonus (crimson flame blade).

Each icon is 20 x 28 pixels like every other buff icon and, like them, is full-bleed art with square corners and a thin
edge: a dark outline one pixel wide, even on all four sides. The art is drawn at 8x
and reduced. Both icons sit in one atlas, 64 x 32 pixels, icon n starting at x = 32 * n (the rest stays transparent).

Output (relative to the client repository root):
  src/bin/Data/Interface/BuffDetails/plus13_icons.OZT        64 x 64
  src/bin/Data/Interface@2x/BuffDetails/plus13_icons.png     128 x 128
  src/bin/Data/Interface@4x/BuffDetails/plus13_icons.png     256 x 256
  tools/plus13_icons/plus13_preview.png                       the icons beside existing buff icons on the buff-bar backdrop

Needs Pillow:  python tools/plus13_icons/make_plus13_icons.py
"""
import math
import os
import random
import struct

from PIL import Image, ImageChops, ImageDraw, ImageFilter

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SS = 8
W, H = 20 * SS, 28 * SS


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def vgrad(size, stops):
    """Vertical gradient through (position 0..1, colour) stops."""
    img = Image.new("RGBA", size)
    px = img.load()
    for y in range(size[1]):
        t = y / max(1, size[1] - 1)
        for i in range(len(stops) - 1):
            (t0, c0), (t1, c1) = stops[i], stops[i + 1]
            if t0 <= t <= t1:
                c = lerp(c0, c1, (t - t0) / max(1e-6, t1 - t0))
                break
        else:
            c = stops[-1][1]
        for x in range(size[0]):
            px[x, y] = c
    return img


def hgrad(size, stops):
    return vgrad((size[1], size[0]), stops).rotate(90, expand=True).transpose(Image.FLIP_LEFT_RIGHT)


def radial(size, center, radius, inner, outer):
    img = Image.new("RGBA", size)
    px = img.load()
    for y in range(size[1]):
        for x in range(size[0]):
            t = min(1.0, math.hypot(x - center[0], y - center[1]) / radius)
            px[x, y] = lerp(inner, outer, t)
    return img


def layer():
    return Image.new("RGBA", (W, H), (0, 0, 0, 0))


def mask_of(draw_fn):
    m = Image.new("L", (W, H), 0)
    draw_fn(ImageDraw.Draw(m))
    return m


def fill(img, mask, source):
    part = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    part.paste(source, (0, 0), mask)
    return Image.alpha_composite(img, part)


def glow(src, radius, strength=1.0):
    blurred = src.filter(ImageFilter.GaussianBlur(radius))
    if strength != 1.0:
        blurred.putalpha(blurred.getchannel("A").point(lambda v: min(255, int(v * strength))))
    return blurred


def spark(img, x, y, r, color=(255, 250, 220)):
    """A four-point glint with a soft halo."""
    halo = layer()
    d = ImageDraw.Draw(halo)
    d.ellipse((x - r * 0.5, y - r * 0.5, x + r * 0.5, y + r * 0.5), fill=color + (200,))
    img = Image.alpha_composite(img, glow(halo, r * 0.7, 1.4))
    d = ImageDraw.Draw(img)
    k = 0.16
    d.polygon([(x, y - r), (x + r * k, y - r * k), (x + r, y), (x + r * k, y + r * k), (x, y + r),
               (x - r * k, y + r * k), (x - r, y), (x - r * k, y - r * k)], fill=color + (255,))
    return img


def rays(img, center, count, length, color, alpha, seed):
    rng = random.Random(seed)
    lay = layer()
    d = ImageDraw.Draw(lay)
    for i in range(count):
        a = 2 * math.pi * i / count + rng.uniform(-0.08, 0.08)
        spread = rng.uniform(0.035, 0.07)
        ln = length * rng.uniform(0.75, 1.0)
        d.polygon([center, (center[0] + ln * math.cos(a - spread), center[1] + ln * math.sin(a - spread)),
                   (center[0] + ln * math.cos(a + spread), center[1] + ln * math.sin(a + spread))],
                  fill=color + (alpha,))
    return Image.alpha_composite(img, lay.filter(ImageFilter.GaussianBlur(3)))


def bevel(img):
    """A plain one-pixel dark edge, the same on all four sides: nothing lighter, nothing extra on any side."""
    e = SS
    dark = (10, 8, 16, 255)
    d = ImageDraw.Draw(img)
    d.rectangle((0, 0, e - 1, H - 1), fill=dark)
    d.rectangle((0, H - e, W - 1, H - 1), fill=dark)
    d.rectangle((0, 0, W - 1, e - 1), fill=dark)
    d.rectangle((W - e, 0, W - 1, H - 1), fill=dark)
    return img


# ---------------------------------------------------------------------------------------------------------------------
# the +13 set: a golden armour crest
# ---------------------------------------------------------------------------------------------------------------------
def shield_outline(cx, top, bottom, half, grow=0):
    pts = []
    n = 24
    for i in range(n + 1):  # right side from the shoulder down to the point
        t = i / n
        x = cx + (half + grow) * math.cos(t * math.pi / 2) ** 0.85
        y = top - grow + (bottom - top + 2 * grow) * (t ** 1.45)
        pts.append((x, y))
    for i in range(n, -1, -1):
        t = i / n
        x = cx - (half + grow) * math.cos(t * math.pi / 2) ** 0.85
        y = top - grow + (bottom - top + 2 * grow) * (t ** 1.45)
        pts.append((x, y))
    return pts


def set_icon():
    cx = W // 2
    img = radial((W, H), (cx, 104), 150, (176, 98, 22, 255), (14, 8, 16, 255))
    img = rays(img, (cx, 104), 18, 190, (255, 214, 120), 70, 7)

    # wings behind the shield
    for sign in (-1, 1):
        wing = layer()
        d = ImageDraw.Draw(wing)
        for k in range(5):
            tip_y = 40 + k * 16
            d.polygon([(cx + sign * 40, 78 + k * 7), (cx + sign * (84 - k * 3), tip_y),
                       (cx + sign * (78 - k * 3), tip_y + 14), (cx + sign * 40, 92 + k * 7)],
                      fill=lerp((255, 232, 150), (196, 130, 34), k / 4) + (255,))
        img = Image.alpha_composite(img, glow(wing, 5, 0.9))
        img = Image.alpha_composite(img, wing)

    top, bottom, half = 56, 196, 56
    # rim: dark outline, gold body with a bright upper-left sheen
    img = fill(img, mask_of(lambda d: d.polygon(shield_outline(cx, top, bottom, half, 7), fill=255)),
               Image.new("RGBA", (W, H), (30, 14, 4, 255)))
    rim = vgrad((W, H), [(0, (255, 240, 170, 255)), (0.45, (226, 164, 44, 255)), (1, (120, 66, 12, 255))])
    img = fill(img, mask_of(lambda d: d.polygon(shield_outline(cx, top, bottom, half, 4), fill=255)), rim)
    img = fill(img, mask_of(lambda d: d.polygon(shield_outline(cx, top, bottom, half, -7), fill=255)),
               Image.new("RGBA", (W, H), (60, 30, 8, 255)))

    # field: deep red and midnight quarters with a gold cross
    field = mask_of(lambda d: d.polygon(shield_outline(cx, top, bottom, half, -11), fill=255))
    quarters = Image.new("RGBA", (W, H), (22, 20, 56, 255))
    qd = ImageDraw.Draw(quarters)
    qd.rectangle((0, 0, cx, 124), fill=(120, 18, 26, 255))
    qd.rectangle((cx, 124, W, H), fill=(120, 18, 26, 255))
    shade = hgrad((W, H), [(0, (255, 255, 255, 40)), (0.5, (0, 0, 0, 0)), (1, (0, 0, 0, 90))])
    quarters = Image.alpha_composite(quarters, shade)
    img = fill(img, field, quarters)

    gold = vgrad((W, H), [(0, (255, 236, 150, 255)), (0.5, (232, 172, 52, 255)), (1, (140, 80, 14, 255))])
    cross = mask_of(lambda d: (d.rectangle((cx - 7, top, cx + 7, bottom), fill=255),
                               d.rectangle((cx - half, 118, cx + half, 132), fill=255)))
    img = fill(img, ImageChops.multiply(field, cross), gold)
    # thin dark edge on the cross for depth
    edge = ImageChops.subtract(cross.filter(ImageFilter.MaxFilter(5)), cross)
    img = fill(img, ImageChops.multiply(field, edge), Image.new("RGBA", (W, H), (50, 24, 6, 255)))

    # centre medallion with a ruby
    d = ImageDraw.Draw(img)
    d.ellipse((cx - 22, 125 - 22, cx + 22, 125 + 22), fill=(40, 18, 6, 255))
    img = fill(img, mask_of(lambda dd: dd.ellipse((cx - 19, 106, cx + 19, 144), fill=255)), gold)
    ruby = radial((W, H), (cx - 4, 119), 20, (255, 150, 150, 255), (150, 6, 24, 255))
    img = fill(img, mask_of(lambda dd: dd.ellipse((cx - 13, 112, cx + 13, 138), fill=255)), ruby)
    img = spark(img, cx - 5, 119, 7)

    # rivets along the rim
    d = ImageDraw.Draw(img)
    for t in (0.06, 0.2, 0.36, 0.55, 0.74):
        y = top + (bottom - top) * t
        x = cx + (half - 1) * math.cos(t * math.pi / 2) ** 0.85
        for sx in (x, 2 * cx - x):
            d.ellipse((sx - 3, y - 3, sx + 3, y + 3), fill=(255, 244, 190, 255), outline=(90, 48, 8, 255))

    # crown above the shield
    crown = layer()
    cd = ImageDraw.Draw(crown)
    base = 54
    cd.polygon([(cx - 30, base), (cx - 34, base - 26), (cx - 16, base - 12), (cx, base - 34), (cx + 16, base - 12),
                (cx + 34, base - 26), (cx + 30, base)], fill=(255, 255, 255, 255))
    crown_mask = crown.getchannel("A")
    ring = ImageChops.subtract(crown_mask.filter(ImageFilter.MaxFilter(7)), crown_mask)
    img = fill(img, ring, Image.new("RGBA", (W, H), (50, 24, 6, 255)))
    img = fill(img, crown_mask, gold)
    d = ImageDraw.Draw(img)
    for gx, gy, col in ((cx - 34, base - 26, (120, 220, 255)), (cx, base - 34, (255, 80, 90)), (cx + 34, base - 26, (120, 220, 255))):
        d.ellipse((gx - 5, gy - 5, gx + 5, gy + 5), fill=col + (255,), outline=(50, 24, 6, 255))
    d.rectangle((cx - 28, base - 8, cx + 28, base - 2), fill=(176, 108, 22, 255))

    # glints
    img = spark(img, cx - 36, 70, 11)
    img = spark(img, cx + 30, 160, 8)
    img = spark(img, cx + 50, 40, 6)
    return bevel(img)


# ---------------------------------------------------------------------------------------------------------------------
# the +13 weapon: a crimson blade in flames
# ---------------------------------------------------------------------------------------------------------------------
def flame_tongue(cx, base_y, height, width, sway):
    pts = [(cx - width, base_y)]
    n = 10
    for i in range(1, n):
        t = i / n
        pts.append((cx - width * (1 - t) ** 0.8 + sway * math.sin(t * 3.4) * t * width, base_y - height * t))
    pts.append((cx + sway * width * 0.5, base_y - height))
    for i in range(n - 1, 0, -1):
        t = i / n
        pts.append((cx + width * (1 - t) ** 0.8 + sway * math.sin(t * 3.4 + 0.8) * t * width, base_y - height * t))
    pts.append((cx + width, base_y))
    return pts


def weapon_icon():
    cx = W // 2
    img = radial((W, H), (cx, 150), 170, (150, 22, 20, 255), (14, 4, 10, 255))
    img = rays(img, (cx, 130), 16, 200, (255, 120, 60), 55, 11)

    # three layers of flame, dark red to white-yellow
    for seed, (col, hs, ws) in enumerate((((120, 10, 14), 1.0, 1.0), ((226, 60, 18), 0.82, 0.8),
                                           ((255, 168, 40), 0.6, 0.55), ((255, 236, 150), 0.38, 0.3))):
        fl = layer()
        d = ImageDraw.Draw(fl)
        rng = random.Random(seed + 3)
        for off, hh in ((-52, 100), (-30, 128), (-8, 104), (14, 132), (36, 104), (56, 96)):
            d.polygon(flame_tongue(cx + off, H - 8, hh * hs * rng.uniform(0.9, 1.05), 26 * ws, rng.uniform(-0.6, 0.6)),
                      fill=col + (240,))
        img = Image.alpha_composite(img, glow(fl, 4, 1.2))
        img = Image.alpha_composite(img, fl.filter(ImageFilter.GaussianBlur(1)))

    # the sword is drawn upright on its own layer and tilted
    sw = layer()
    d = ImageDraw.Draw(sw)
    tip, guard_y, grip_y = 16, 130, 176
    left, right = cx - 21, cx + 21
    blade = [(cx, tip), (right, tip + 34), (right, guard_y), (left, guard_y), (left, tip + 34)]
    blade_mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(blade_mask).polygon(blade, fill=255)
    # dark outline
    ol = blade_mask.filter(ImageFilter.MaxFilter(9))
    sw = fill(sw, ol, Image.new("RGBA", (W, H), (30, 4, 10, 255)))
    steel = hgrad((W, H), [(0, (255, 240, 232, 255)), (0.5, (246, 104, 100, 255)), (0.51, (190, 36, 54, 255)), (1, (120, 16, 36, 255))])
    steel = Image.alpha_composite(steel, vgrad((W, H), [(0, (255, 255, 255, 70)), (0.6, (0, 0, 0, 0)), (1, (0, 0, 0, 60))]))
    sw = fill(sw, blade_mask, steel)
    d = ImageDraw.Draw(sw)
    d.line([(cx, tip + 6), (cx, guard_y - 6)], fill=(70, 4, 18, 255), width=6)   # fuller
    d.line([(cx - 3, tip + 38), (cx - 3, guard_y - 10)], fill=(255, 190, 170, 200), width=2)
    # glowing runes in the fuller
    runes = layer()
    rd = ImageDraw.Draw(runes)
    ry = tip + 48
    glyphs = [((-5, 0), (5, 0), (0, 10)), ((-5, 0), (0, 10), (5, 0), (0, 0)), ((0, -2), (0, 10), (-5, 4), (5, 4))]
    k = 0
    while ry < guard_y - 18:
        g = glyphs[k % 3]
        rd.line([(cx + x, ry + y) for x, y in g], fill=(255, 200, 70, 255), width=3)
        ry += 17
        k += 1
    sw = Image.alpha_composite(sw, glow(runes, 5, 1.8))
    sw = Image.alpha_composite(sw, runes)

    # guard with curved quillons, wrapped grip, jewelled pommel
    gold = vgrad((W, H), [(0, (255, 238, 160, 255)), (0.5, (226, 160, 40, 255)), (1, (120, 66, 12, 255))])
    guard = mask_of(lambda dd: (dd.polygon([(cx - 52, guard_y - 6), (cx - 34, guard_y - 14), (cx - 12, guard_y - 6),
                                            (cx + 12, guard_y - 6), (cx + 34, guard_y - 14), (cx + 52, guard_y - 6),
                                            (cx + 40, guard_y + 8), (cx + 16, guard_y + 14), (cx - 16, guard_y + 14),
                                            (cx - 40, guard_y + 8)], fill=255)))
    sw = fill(sw, guard.filter(ImageFilter.MaxFilter(7)), Image.new("RGBA", (W, H), (40, 20, 4, 255)))
    sw = fill(sw, guard, gold)
    d = ImageDraw.Draw(sw)
    d.ellipse((cx - 7, guard_y - 4, cx + 7, guard_y + 10), fill=(220, 20, 50, 255), outline=(60, 10, 10, 255), width=2)
    d.rectangle((cx - 8, guard_y + 12, cx + 8, grip_y), fill=(70, 22, 22, 255), outline=(30, 8, 8, 255), width=2)
    for yy in range(guard_y + 16, grip_y, 9):
        d.line([(cx - 8, yy), (cx + 8, yy - 6)], fill=(208, 160, 60, 255), width=3)
    d.ellipse((cx - 12, grip_y - 2, cx + 12, grip_y + 22), fill=(230, 160, 40, 255), outline=(60, 30, 6, 255), width=3)
    d.ellipse((cx - 5, grip_y + 4, cx + 5, grip_y + 14), fill=(255, 90, 110, 255))

    sw = sw.rotate(-9, resample=Image.BICUBIC, center=(cx, 150))
    halo = fill(layer(), ImageChops.multiply(sw.getchannel('A').filter(ImageFilter.MaxFilter(15)), Image.new('L', (W, H), 255)), Image.new('RGBA', (W, H), (255, 190, 120, 255)))
    img = Image.alpha_composite(img, glow(halo, 9, 0.75))
    img = Image.alpha_composite(img, glow(sw, 8, 0.8))
    img = Image.alpha_composite(img, sw)

    # embers and a glint on the tip
    rng = random.Random(5)
    d = ImageDraw.Draw(img)
    for _ in range(26):
        x, y, r = rng.randint(14, W - 14), rng.randint(30, H - 30), rng.randint(2, 4)
        d.ellipse((x - r, y - r, x + r, y + r), fill=(255, rng.randint(150, 230), 70, 255))
    img = spark(img, cx + 30, 30, 12)
    return bevel(img)


# ---------------------------------------------------------------------------------------------------------------------
CELL = 32      # texels between two icons
PAD = 4        # empty-free border around an icon inside its cell: the icon's own edge colours are repeated there, so
               # the texture filter never mixes the icon edge with a neighbour or with transparent texels
ATLAS_W, ATLAS_H = 64, 64


def atlas(scale):
    """The atlas at 1x/2x/4x. Icon n sits at (CELL * n + PAD, PAD); its cell is filled with the repeated edge pixels."""
    icons = [set_icon(), weapon_icon()]
    sheet = Image.new("RGBA", (ATLAS_W * scale, ATLAS_H * scale), (0, 0, 0, 0))
    iw, ih = 20 * scale, 28 * scale
    for n, icon in enumerate(icons):
        small = icon.resize((iw, ih), Image.LANCZOS)
        x0, y0 = (CELL * n + PAD) * scale, PAD * scale
        cell_w, cell_h = CELL * scale, ATLAS_H * scale
        cx0 = CELL * n * scale
        padded = Image.new("RGBA", (cell_w, cell_h))
        # repeat the outermost rows and columns of the icon over the cell
        padded.paste(small, (x0 - cx0, y0))
        left = small.crop((0, 0, 1, ih)).resize((x0 - cx0, ih))
        right = small.crop((iw - 1, 0, iw, ih)).resize((cell_w - (x0 - cx0) - iw, ih))
        padded.paste(left, (0, y0))
        padded.paste(right, (x0 - cx0 + iw, y0))
        top = padded.crop((0, y0, cell_w, y0 + 1)).resize((cell_w, y0))
        bottom = padded.crop((0, y0 + ih - 1, cell_w, y0 + ih)).resize((cell_w, cell_h - y0 - ih))
        padded.paste(top, (0, 0))
        padded.paste(bottom, (0, y0 + ih))
        sheet.paste(padded, (cx0, 0))
    return sheet


def png_to_ozt(image, destination):
    nx, ny = image.size
    px = image.load()
    header = bytes([0, 0, 2]) + bytes(5) + bytes(2) + bytes(2) + bytes(4)
    header += struct.pack("<h", nx) + struct.pack("<h", ny) + bytes([32, 0])
    body = bytearray()
    for y in range(ny - 1, -1, -1):
        for x in range(nx):
            r, g, b, a = px[x, y]
            body += bytes([b, g, r, a])
    with open(destination, "wb") as f:
        f.write(header)
        f.write(body)
        f.write(bytes(8) + b"TRUEVISION-XFILE.\x00")


def preview(master, small, data):
    """The two icons next to existing buff icons (same sheets the game uses), enlarged, on the buff bar backdrop."""
    sheet = Image.open(os.path.join(data, "Interface@4x", "BuffDetails", "newui_statusicon.png")).convert("RGBA")
    sheet2 = Image.open(os.path.join(data, "Interface@4x", "BuffDetails", "newui_statusicon2.png")).convert("RGBA")
    cw, ch = 80, 112

    def existing(img, i):
        return img.crop(((i % 10) * cw, (i // 10) * ch, (i % 10) * cw + cw, (i // 10) * ch + ch))

    others = [existing(sheet, 27), existing(sheet2, 9), existing(sheet, 5), existing(sheet, 2)]
    mine = [master.crop(((CELL * n + PAD) * 4, PAD * 4, (CELL * n + PAD) * 4 + cw, PAD * 4 + ch)) for n in range(2)]
    row = others + mine
    big = Image.new("RGBA", (len(row) * 110 + 20, 150), (36, 38, 46, 255))
    for i, ic in enumerate(row):
        big.alpha_composite(ic, (20 + i * 110, 20))
    # the same row at real size (x1, and x3 enlarged) so the game look is judged at its true scale
    strip = Image.new("RGBA", (len(row) * 24 + 8, 36), (36, 38, 46, 255))
    for i, ic in enumerate(row):
        strip.alpha_composite(ic.resize((15, 21), Image.LANCZOS), (8 + i * 24, 8))
    strip = strip.resize((strip.width * 5, strip.height * 5), Image.NEAREST)
    out = Image.new("RGBA", (max(big.width, strip.width), big.height + strip.height), (36, 38, 46, 255))
    out.alpha_composite(big, (0, 0))
    out.alpha_composite(strip, (0, big.height))
    return out


def main():
    data = os.path.join(REPO, "src", "bin", "Data")
    os.makedirs(os.path.join(data, "Interface@2x", "BuffDetails"), exist_ok=True)
    os.makedirs(os.path.join(data, "Interface@4x", "BuffDetails"), exist_ok=True)
    png_to_ozt(atlas(1), os.path.join(data, "Interface", "BuffDetails", "plus13_icons.OZT"))
    atlas(2).save(os.path.join(data, "Interface@2x", "BuffDetails", "plus13_icons.png"))
    master = atlas(4)
    master.save(os.path.join(data, "Interface@4x", "BuffDetails", "plus13_icons.png"))
    preview(master, atlas(1), data).save(os.path.join(REPO, "tools", "plus13_icons", "plus13_preview.png"))
    print("wrote the +13 icon atlas")


if __name__ == "__main__":
    main()
