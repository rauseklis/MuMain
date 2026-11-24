"""Generates the two +13 buff icons: the full-set bonus (golden shield) and the weapon bonus (crimson blade).

Each icon is 20 x 28 pixels like the other buff icons, drawn at 8x and reduced. Both sit in one atlas, 64 x 32 pixels,
icon n starting at x = 32 * n (the part right of and below the icon stays transparent).

Output (relative to the client repository root):
  src/bin/Data/Interface/BuffDetails/plus13_icons.OZT        64 x 32
  src/bin/Data/Interface@2x/BuffDetails/plus13_icons.png     128 x 64
  src/bin/Data/Interface@4x/BuffDetails/plus13_icons.png     256 x 128
  tools/plus13_icons/plus13_preview.png                       the two icons enlarged on a dark buff-bar backdrop

Needs Pillow:  python tools/plus13_icons/make_plus13_icons.py
"""
import math
import os
import struct

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SS = 8
W, H = 20 * SS, 28 * SS


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


def vertical_gradient(size, top, bottom):
    img = Image.new("RGBA", size)
    px = img.load()
    for y in range(size[1]):
        c = lerp(top, bottom, y / max(1, size[1] - 1))
        for x in range(size[0]):
            px[x, y] = c
    return img


def masked(fill, mask):
    out = Image.new("RGBA", fill.size, (0, 0, 0, 0))
    out.paste(fill, (0, 0), mask)
    return out


def glow(layer, radius, strength=1.0):
    blurred = layer.filter(ImageFilter.GaussianBlur(radius))
    if strength != 1.0:
        a = blurred.getchannel("A").point(lambda v: min(255, int(v * strength)))
        blurred.putalpha(a)
    return blurred


def font(size):
    for name in ("georgiab.ttf", "timesbd.ttf", "arialbd.ttf", "DejaVuSerif-Bold.ttf"):
        for base in (r"C:\Windows\Fonts", "/usr/share/fonts/truetype/dejavu"):
            path = os.path.join(base, name)
            if os.path.exists(path):
                return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def plate():
    """Dark plate with a double gold frame, shared by both icons."""
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    mask = Image.new("L", (W, H), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, W - 1, H - 1), radius=22, fill=255)
    back = vertical_gradient((W, H), (30, 26, 40, 255), (8, 6, 12, 255))
    img.paste(back, (0, 0), mask)
    return img, mask


def frame(img, outer, inner):
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((0, 0, W - 1, H - 1), radius=22, outline=outer, width=9)
    d.rounded_rectangle((10, 10, W - 11, H - 11), radius=14, outline=inner, width=3)


def shield_points(cx, top, bottom, half):
    pts = []
    # flat top with a small notch, straight shoulders, curved sides to a point
    pts += [(cx - half, top), (cx - half * 0.35, top + 8), (cx, top), (cx + half * 0.35, top + 8), (cx + half, top)]
    for i in range(1, 17):
        t = i / 16
        x = cx + half * math.cos(t * math.pi / 2) * (1 - 0.0 * t)
        y = top + (bottom - top) * (t ** 1.35)
        pts.append((x, y))
    for i in range(15, 0, -1):
        t = i / 16
        x = cx - half * math.cos(t * math.pi / 2)
        y = top + (bottom - top) * (t ** 1.35)
        pts.append((x, y))
    return pts


def number_layer(color, outline, size, center, text="13"):
    layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    f = font(size)
    box = d.textbbox((0, 0), text, font=f)
    x = center[0] - (box[0] + box[2]) / 2
    y = center[1] - (box[1] + box[3]) / 2
    d.text((x, y), text, font=f, fill=color, stroke_width=7, stroke_fill=outline)
    return layer


def set_icon():
    img, mask = plate()
    cx = W // 2
    # soft golden aura behind the shield
    aura = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(aura).ellipse((14, 40, W - 14, H - 34), fill=(255, 190, 40, 200))
    img = Image.alpha_composite(img, glow(aura, 26, 0.9))

    pts = shield_points(cx, 34, H - 26, 62)
    outer = Image.new("L", (W, H), 0)
    ImageDraw.Draw(outer).polygon(pts, fill=255)
    gold = vertical_gradient((W, H), (255, 236, 150, 255), (176, 108, 18, 255))
    img = Image.alpha_composite(img, masked(gold, outer))

    # inner field, darker with a faint vertical sheen
    inner_pts = shield_points(cx, 50, H - 44, 46)
    inner = Image.new("L", (W, H), 0)
    ImageDraw.Draw(inner).polygon(inner_pts, fill=255)
    field = vertical_gradient((W, H), (132, 84, 22, 255), (52, 26, 8, 255))
    img = Image.alpha_composite(img, masked(field, inner))

    # rim highlight on the left edge
    hi = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(hi).line([(cx - 60, 40), (cx - 54, 110)], fill=(255, 250, 220, 220), width=5)
    img = Image.alpha_composite(img, hi.filter(ImageFilter.GaussianBlur(2)))

    # glowing 13
    num = number_layer((255, 246, 200, 255), (110, 56, 0, 255), 100, (cx, 122))
    img = Image.alpha_composite(img, glow(num, 12, 1.6))
    img = Image.alpha_composite(img, num)

    # small crest star on top
    d = ImageDraw.Draw(img)
    sx, sy, r = cx, 20, 10
    star = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        rr = r if i % 2 == 0 else r * 0.42
        star.append((sx + rr * math.cos(a), sy + rr * math.sin(a)))
    d.polygon(star, fill=(255, 244, 190, 255))

    frame(img, (214, 160, 40, 255), (92, 58, 12, 255))
    img.putalpha(ImageChops.multiply(img.getchannel("A"), mask))
    return img


def flame(layer, cx, base_y, height, width, color):
    d = ImageDraw.Draw(layer)
    d.polygon([(cx - width, base_y), (cx - width * 0.55, base_y - height * 0.55), (cx - width * 0.2, base_y - height * 0.42),
               (cx, base_y - height), (cx + width * 0.25, base_y - height * 0.5), (cx + width * 0.6, base_y - height * 0.6),
               (cx + width, base_y)], fill=color)


def weapon_icon():
    img, mask = plate()
    cx = W // 2

    # fire behind the blade
    fire = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    flame(fire, cx - 30, H - 24, 150, 44, (190, 20, 20, 235))
    flame(fire, cx + 28, H - 24, 170, 46, (210, 40, 16, 235))
    flame(fire, cx, H - 20, 130, 36, (255, 120, 20, 235))
    flame(fire, cx, H - 20, 70, 20, (255, 214, 90, 240))
    img = Image.alpha_composite(img, glow(fire, 9, 1.4))
    img = Image.alpha_composite(img, fire.filter(ImageFilter.GaussianBlur(1.5)))

    # blade: vertical, point up, slightly diagonal
    blade = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    bd = ImageDraw.Draw(blade)
    pts = [(cx + 6, 14), (cx + 18, 34), (cx + 18, 126), (cx - 14, 126), (cx - 14, 34)]
    pts = [(x - 6 + (126 - y) * 0.0, y) for x, y in pts]
    bd.polygon(pts, fill=(226, 70, 70, 255))
    # lit edge and dark fuller
    bd.polygon([(cx + 6, 14), (cx + 12, 34), (cx + 12, 126), (cx + 2, 126), (cx + 2, 34)], fill=(255, 200, 180, 255))
    bd.polygon([(cx - 8, 36), (cx - 1, 36), (cx - 1, 124), (cx - 8, 124)], fill=(120, 14, 22, 255))
    img = Image.alpha_composite(img, glow(blade, 7, 1.2))
    img = Image.alpha_composite(img, blade)

    # gold crossguard + grip + pommel
    gd = ImageDraw.Draw(img)
    gd.rounded_rectangle((cx - 48, 124, cx + 36, 140), radius=6, fill=(236, 186, 60, 255), outline=(120, 74, 10, 255), width=3)
    gd.rectangle((cx - 9, 140, cx + 3, 154), fill=(110, 40, 30, 255), outline=(60, 22, 14, 255), width=2)
    gd.ellipse((cx - 14, 152, cx + 8, 168), fill=(236, 186, 60, 255), outline=(120, 74, 10, 255), width=3)

    # glowing 13 across the lower half
    num = number_layer((255, 236, 210, 255), (96, 8, 14, 255), 80, (cx - 3, 188))
    img = Image.alpha_composite(img, glow(num, 11, 1.7))
    img = Image.alpha_composite(img, num)

    frame(img, (190, 40, 40, 255), (240, 180, 70, 255))
    img.putalpha(ImageChops.multiply(img.getchannel("A"), mask))
    return img


def atlas(scale):
    """The atlas at 1x/2x/4x; the drawn icons are reduced from the 8x masters."""
    icons = [set_icon(), weapon_icon()]
    sheet = Image.new("RGBA", (64 * scale, 32 * scale), (0, 0, 0, 0))
    for n, icon in enumerate(icons):
        small = icon.resize((20 * scale, 28 * scale), Image.LANCZOS)
        sheet.paste(small, (32 * scale * n, 0))
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


def main():
    data = os.path.join(REPO, "src", "bin", "Data")
    os.makedirs(os.path.join(data, "Interface@2x", "BuffDetails"), exist_ok=True)
    os.makedirs(os.path.join(data, "Interface@4x", "BuffDetails"), exist_ok=True)
    png_to_ozt(atlas(1), os.path.join(data, "Interface", "BuffDetails", "plus13_icons.OZT"))
    atlas(2).save(os.path.join(data, "Interface@2x", "BuffDetails", "plus13_icons.png"))
    atlas(4).save(os.path.join(data, "Interface@4x", "BuffDetails", "plus13_icons.png"))

    master = atlas(4)
    backdrop = Image.new("RGBA", (2 * 140 + 30, 170), (38, 40, 48, 255))
    for n in range(2):
        icon = master.crop((32 * 4 * n, 0, 32 * 4 * n + 80, 112))
        backdrop.alpha_composite(icon.resize((100, 140), Image.LANCZOS), (20 + n * 150, 15))
    small = atlas(1)
    for n in range(2):
        tile = small.crop((32 * n, 0, 32 * n + 20, 28))
        backdrop.alpha_composite(tile, (125 + n * 150, 70))
    backdrop = backdrop.resize((backdrop.width * 2, backdrop.height * 2), Image.NEAREST)
    backdrop.save(os.path.join(REPO, "tools", "plus13_icons", "plus13_preview.png"))
    print("wrote the +13 icon atlas")


if __name__ == "__main__":
    main()
