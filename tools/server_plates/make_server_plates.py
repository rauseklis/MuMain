"""Draws the server select plates (Ember 5x / Inferno 500x): dark glass hexagon plates with a glowing rune line, a crest
in a hexagon frame and the name with an "EXPERIENCE" line underneath.

The atlas holds one column per server (Ember, Inferno) and four rows: idle, hover, pressed and the rune glow layer that
the client pulses over the plate. A cell is 208 x 48 pixels at 1x (the 200 x 40 plate plus 4 pixels of shadow room).
Everything is drawn at 8x and reduced.

Output (relative to the client repository root):
  src/bin/Data/Interface/server_plates.OZT          416 x 192
  src/bin/Data/Interface@2x/server_plates.png       832 x 384
  src/bin/Data/Interface@4x/server_plates.png       1664 x 768

Needs Pillow and numpy:  python tools/server_plates/make_server_plates.py
"""
import os
import struct

import math
import random
import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

SS = 8
FONTS = r'C:\Windows\Fonts'


def font(name, px):
    return ImageFont.truetype(FONTS + '\\' + name, int(round(px * SS)))


def lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(len(a)))


def blank(w, h, color=(0, 0, 0, 0)):
    return Image.new('RGBA', (int(w * SS), int(h * SS)), color)


def poly_mask(size, pts):
    m = Image.new('L', size, 0)
    ImageDraw.Draw(m).polygon([(x * SS, y * SS) for x, y in pts], fill=255)
    return m


def rect_mask(size, x0, y0, x1, y1, cut=0.0, cut2=None):
    cut2 = cut if cut2 is None else cut2
    pts = [(x0 + cut, y0), (x1 - cut2, y0), (x1, y0 + cut2), (x1, y1 - cut2), (x1 - cut2, y1), (x0 + cut, y1),
           (x0, y1 - cut), (x0, y0 + cut)]
    return poly_mask(size, pts)


def vgrad(size, stops):
    """stops: [(t, (r,g,b,a)), ...] vertical gradient RGBA image."""
    w, h = size
    arr = np.zeros((h, w, 4), np.float32)
    ys = np.linspace(0, 1, h)
    ts = [s[0] for s in stops]
    for c in range(4):
        col = np.interp(ys, ts, [s[1][c] for s in stops])
        arr[:, :, c] = col[:, None]
    return Image.fromarray(arr.clip(0, 255).astype(np.uint8), 'RGBA')


def hgrad(size, stops):
    w, h = size
    arr = np.zeros((h, w, 4), np.float32)
    xs = np.linspace(0, 1, w)
    ts = [s[0] for s in stops]
    for c in range(4):
        col = np.interp(xs, ts, [s[1][c] for s in stops])
        arr[:, :, c] = col[None, :]
    return Image.fromarray(arr.clip(0, 255).astype(np.uint8), 'RGBA')


def radial(size, cx, cy, rx, ry, color, power=1.6):
    w, h = size
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.sqrt(((xx - cx * SS) / (rx * SS)) ** 2 + ((yy - cy * SS) / (ry * SS)) ** 2)
    a = np.clip(1 - d, 0, 1) ** power * color[3]
    arr = np.zeros((h, w, 4), np.uint8)
    arr[:, :, 0], arr[:, :, 1], arr[:, :, 2] = color[0], color[1], color[2]
    arr[:, :, 3] = a.clip(0, 255).astype(np.uint8)
    return Image.fromarray(arr, 'RGBA')


def masked(fill, mask):
    out = Image.new('RGBA', fill.size, (0, 0, 0, 0))
    a = ImageChops.multiply(fill.getchannel('A'), mask)
    out.paste(fill.convert('RGB'), (0, 0))
    out.putalpha(a)
    return out


def over(base, layer):
    base.alpha_composite(layer)
    return base


def add_glow(base, layer, radius, k=1.0):
    """Additive light: blur the layer, add its colour*alpha onto base."""
    b = layer.filter(ImageFilter.GaussianBlur(radius * SS))
    arr_b = np.asarray(b).astype(np.float32)
    a = arr_b[:, :, 3:4] / 255.0 * k
    arr = np.asarray(base).astype(np.float32)
    arr[:, :, :3] = np.clip(arr[:, :, :3] + arr_b[:, :, :3] * a, 0, 255)
    base.paste(Image.fromarray(arr.astype(np.uint8), 'RGBA'))
    return base


def noise_streaks(size, strength, seed, along='x', scale=60):
    rnd = np.random.RandomState(seed)
    w, h = size
    if along == 'x':
        small = rnd.rand(h, max(2, w // (scale * SS // 8))).astype(np.float32)
    else:
        small = rnd.rand(max(2, h // (scale * SS // 8)), w).astype(np.float32)
    im = Image.fromarray((small * 255).astype(np.uint8), 'L').resize((w, h), Image.BICUBIC)
    fine = rnd.rand(h, w).astype(np.float32)
    arr = (np.asarray(im).astype(np.float32) / 255 - 0.5) * strength + (fine - 0.5) * strength * 0.5
    return arr


def grain(size, strength, seed):
    rnd = np.random.RandomState(seed)
    w, h = size
    base = rnd.rand(h // 3 + 1, w // 3 + 1).astype(np.float32)
    im = np.asarray(Image.fromarray((base * 255).astype(np.uint8), 'L').resize((w, h), Image.BICUBIC)).astype(np.float32) / 255
    fine = rnd.rand(h, w).astype(np.float32)
    return ((im - 0.5) * 0.9 + (fine - 0.5) * 0.7) * strength


def tint_noise(fill_rgb, noise, size):
    w, h = size
    arr = np.zeros((h, w, 4), np.float32)
    for c in range(3):
        arr[:, :, c] = fill_rgb[c] + noise
    arr[:, :, 3] = 255
    return Image.fromarray(arr.clip(0, 255).astype(np.uint8), 'RGBA')


def edge_light(mask, dx, dy, blur=0.0):
    """Light that catches the edge facing (-dx,-dy): mask minus mask shifted by (dx,dy)."""
    shifted = ImageChops.offset(mask, int(dx * SS), int(dy * SS))
    # offset wraps; fine because masks never touch the canvas border
    diff = ImageChops.subtract(mask, shifted)
    if blur:
        diff = diff.filter(ImageFilter.GaussianBlur(blur * SS))
    return diff


def bevel(base, mask, light=(255, 255, 255), dark=(0, 0, 0), depth=1.0, a_light=150, a_dark=170):
    hi = edge_light(mask, depth, depth, 0.2)
    lo = edge_light(mask, -depth, -depth, 0.2)
    hl = Image.new('RGBA', base.size, light + (0,))
    hl.putalpha(hi.point(lambda v: int(v * a_light / 255)))
    dl = Image.new('RGBA', base.size, dark + (0,))
    dl.putalpha(lo.point(lambda v: int(v * a_dark / 255)))
    over(base, hl)
    over(base, dl)
    return base


def inner_shadow(base, mask, radius, strength, color=(0, 0, 0)):
    inv = ImageChops.invert(mask).filter(ImageFilter.GaussianBlur(radius * SS))
    sh = Image.new('RGBA', base.size, color + (0,))
    sh.putalpha(ImageChops.multiply(inv, mask).point(lambda v: int(min(255, v * strength))))
    over(base, sh)
    return base


def emboss_text(base, text, fnt, cx, cy, top=(255, 236, 160), bottom=(170, 112, 30), outline=(40, 20, 6),
                glow_color=None, glow_r=1.4, glow_k=0.9, spacing=0.0, anchor='mm', shadow=(0, 0, 0, 170)):
    """Embossed gold lettering centred at (cx, cy) in 1x units."""
    layer = Image.new('L', base.size, 0)
    d = ImageDraw.Draw(layer)
    if spacing:
        total = sum(d.textlength(ch, font=fnt) for ch in text) + spacing * SS * (len(text) - 1)
        x = cx * SS - total / 2
        for ch in text:
            d.text((x, cy * SS), ch, font=fnt, fill=255, anchor='lm')
            x += d.textlength(ch, font=fnt) + spacing * SS
    else:
        d.text((cx * SS, cy * SS), text, font=fnt, fill=255, anchor=anchor)
    # drop shadow
    sh = ImageChops.offset(layer, int(0.7 * SS), int(0.9 * SS)).filter(ImageFilter.GaussianBlur(0.5 * SS))
    s = Image.new('RGBA', base.size, shadow[:3] + (0,))
    s.putalpha(sh.point(lambda v: int(v * shadow[3] / 255)))
    over(base, s)
    if glow_color is not None:
        g = Image.new('RGBA', base.size, glow_color + (0,))
        g.putalpha(layer)
        add_glow(base, g, glow_r, glow_k)
    # outline
    ol = layer.filter(ImageFilter.MaxFilter(3))
    o = Image.new('RGBA', base.size, outline + (0,))
    o.putalpha(ol)
    over(base, o)
    # gold fill: vertical gradient limited to text bbox
    bb = layer.getbbox()
    if bb:
        y0, y1 = bb[1], bb[3]
        grad = Image.new('RGBA', base.size, (0, 0, 0, 0))
        g1 = vgrad((base.size[0], max(2, y1 - y0)), [(0, top + (255,)), (0.55, lerp(top, bottom, 0.55) + (255,)),
                                                     (1, bottom + (255,))])
        grad.paste(g1, (0, y0))
        grad = masked(grad, layer)
        over(base, grad)
        # top highlight line (emboss)
        hi = edge_light(layer, 0, 0.8, 0.0)
        h = Image.new('RGBA', base.size, (255, 250, 220, 0))
        h.putalpha(hi.point(lambda v: int(v * 0.7)))
        over(base, h)
    return base


def flame_poly(cx, by, h, w, lean=0.0, curl=0.0, steps=40):
    left, right = [], []
    for i in range(steps + 1):
        t = i / steps
        hw = w * (math.sin(math.pi * min(1, t * 0.92 + 0.04)) ** 0.9) * (1 - t) ** 0.55
        bend = lean * t ** 1.8 * h * 0.5 + curl * math.sin(t * math.pi * 1.2) * w * 0.8
        y = by - t * h
        left.append((cx - hw + bend, y))
        right.append((cx + hw + bend, y))
    return left + right[::-1]


def draw_flame(base, cx, by, h, w, lean=0.0, curl=0.0, palette=None, glow_k=0.9):
    palette = palette or [((120, 12, 6), 1.0), ((235, 70, 14), 0.78), ((255, 170, 40), 0.56), ((255, 244, 190), 0.30)]
    for col, s in palette:
        pts = flame_poly(cx, by, h * s, w * s, lean * s, curl * s)
        m = poly_mask(base.size, pts).filter(ImageFilter.GaussianBlur(0.12 * SS))
        L = Image.new('RGBA', base.size, col + (0,))
        L.putalpha(m)
        over(base, L)
    return base


def sparks(base, rect, n, seed, colors, size=(0.5, 1.1), glow_r=0.9):
    rnd = random.Random(seed)
    x0, y0, x1, y1 = rect
    layer = Image.new('RGBA', base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    for _ in range(n):
        x = rnd.uniform(x0, x1) * SS
        y = rnd.uniform(y0, y1) * SS
        r = rnd.uniform(*size) * SS * 0.5
        c = rnd.choice(colors)
        d.ellipse((x - r, y - r, x + r, y + r), fill=c + (255,))
    over(base, layer)
    add_glow(base, layer, glow_r, 1.2)
    return base


def downscale(im, factor):
    w, h = im.size
    return im.resize((int(w / SS * factor), int(h / SS * factor)), Image.LANCZOS)



PW, PH = 200, 40     # the plate, in 1x pixels
M = 4                # shadow room around it
CW, CH = PW + 2 * M, PH + 2 * M

THEMES = {
    'ember': dict(label='EMBER', sub='EXPERIENCE  x5', glow=(255, 120, 24), glow2=(255, 190, 70),
                  panel=(52, 30, 18), sub_col=(255, 196, 120), gem=(255, 150, 30)),
    'inferno': dict(label='INFERNO', sub='EXPERIENCE  x500', glow=(230, 30, 34), glow2=(255, 96, 60),
                    panel=(50, 14, 16), sub_col=(255, 150, 130), gem=(240, 40, 40)),
}

def outline(mask, px):
    k = max(3, int(px * SS) | 1)
    return ImageChops.subtract(mask, mask.filter(ImageFilter.MinFilter(k)))

def crest_ember(base, cx, cy, s, glow):
    # a three-tongue flame crest
    L = Image.new('RGBA', base.size, (0, 0, 0, 0))
    by = cy + 8 * s
    draw_flame(L, cx - 4.6 * s, by, 11 * s, 3.2 * s, lean=-0.55, curl=0.25)
    draw_flame(L, cx + 4.6 * s, by, 11 * s, 3.2 * s, lean=0.55, curl=-0.25)
    draw_flame(L, cx, by + 0.6 * s, 17 * s, 5.0 * s, lean=0.12, curl=0.3)
    over(base, L)
    add_glow(base, L, 1.6, 0.9)
    return base

def crest_inferno(base, cx, cy, s, glow):
    # flames behind a horned demon head
    L = Image.new('RGBA', base.size, (0, 0, 0, 0))
    pal = [((90, 6, 8), 1.0), ((200, 24, 20), 0.8), ((255, 90, 40), 0.55)]
    draw_flame(L, cx - 6.5 * s, cy + 9 * s, 14 * s, 3.4 * s, lean=-0.6, curl=0.2, palette=pal)
    draw_flame(L, cx + 6.5 * s, cy + 9 * s, 14 * s, 3.4 * s, lean=0.6, curl=-0.2, palette=pal)
    over(base, L)
    add_glow(base, L, 1.6, 0.8)
    # horns
    def bez(p0, p1, p2, p3, n=22):
        pts = []
        for i in range(n + 1):
            t = i / n
            x = (1 - t) ** 3 * p0[0] + 3 * (1 - t) ** 2 * t * p1[0] + 3 * (1 - t) * t ** 2 * p2[0] + t ** 3 * p3[0]
            y = (1 - t) ** 3 * p0[1] + 3 * (1 - t) ** 2 * t * p1[1] + 3 * (1 - t) * t ** 2 * p2[1] + t ** 3 * p3[1]
            pts.append((x, y))
        return pts
    horn_l = bez((-3.2, -2.0), (-8.0, -1.0), (-10.5, -5.0), (-9.0, -10.5)) + \
        bez((-9.0, -10.5), (-8.2, -6.2), (-6.0, -3.4), (-1.8, -4.6))[::-1][::-1]
    horn_l = bez((-3.0, -1.6), (-9.0, 0.0), (-11.5, -5.5), (-8.8, -11.0)) + \
        bez((-8.8, -11.0), (-8.6, -6.5), (-6.5, -4.0), (-2.2, -5.6))
    horn_r = [(-x, y) for x, y in horn_l]
    head = [(-4.2, -4.6), (4.2, -4.6), (4.6, 0.5), (3.0, 5.0), (0, 8.2), (-3.0, 5.0), (-4.6, 0.5)]
    for pts, col_dark, col_hi in ((horn_l, (60, 22, 20), (190, 80, 60)), (horn_r, (60, 22, 20), (190, 80, 60))):
        P = [(cx + x * s, cy + y * s) for x, y in pts]
        m = poly_mask(base.size, P)
        F = Image.new('RGBA', base.size, col_dark + (0,))
        F.putalpha(m)
        over(base, F)
        bevel(base, m, light=col_hi, dark=(0, 0, 0), depth=0.6, a_light=190, a_dark=160)
    P = [(cx + x * s, cy + y * s) for x, y in head]
    m = poly_mask(base.size, P)
    F = vgrad(base.size, [(0, (74, 70, 76, 255)), (1, (22, 20, 26, 255))])
    over(base, masked(F, m))
    bevel(base, m, light=(255, 150, 120), dark=(0, 0, 0), depth=0.7, a_light=200, a_dark=200)
    # glowing eyes and brow
    E = Image.new('RGBA', base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(E)
    for sg in (-1, 1):
        d.polygon([((cx + sg * 0.9 * s) * SS, (cy - 0.6 * s) * SS), ((cx + sg * 3.7 * s) * SS, (cy - 2.4 * s) * SS),
                   ((cx + sg * 3.5 * s) * SS, (cy - 0.7 * s) * SS), ((cx + sg * 1.2 * s) * SS, (cy + 0.5 * s) * SS)],
                  fill=(255, 226, 120, 255))
    over(base, E)
    add_glow(base, E, 0.9, 1.4)
    # fangs / jaw line
    J = Image.new('RGBA', base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(J)
    for sg in (-1, 1):
        d.polygon([((cx + sg * 1.2 * s) * SS, (cy + 4.2 * s) * SS), ((cx + sg * 2.4 * s) * SS, (cy + 4.2 * s) * SS),
                   ((cx + sg * 1.8 * s) * SS, (cy + 6.6 * s) * SS)], fill=(220, 210, 190, 255))
    over(base, J)
    return base

def stroke_path(base, pts, width, color, closed=True):
    L = Image.new('RGBA', base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(L)
    P = [(x * SS, y * SS) for x, y in pts]
    if closed:
        P = P + [P[0]]
    d.line(P, fill=color, width=int(width * SS), joint='curve')
    return L

def rivet(base, cx, cy, r, col_hi=(255, 236, 170), col_lo=(120, 78, 20)):
    m = Image.new('L', base.size, 0)
    ImageDraw.Draw(m).ellipse(((cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS), fill=255)
    sh = ImageChops.offset(m, 0, int(0.8 * SS)).filter(ImageFilter.GaussianBlur(0.5 * SS))
    S = Image.new('RGBA', base.size, (0, 0, 0, 0))
    S.putalpha(sh.point(lambda v: int(v * 0.8)))
    over(base, S)
    G = vgrad(base.size, [(0, col_hi + (255,)), (1, col_lo + (255,))])
    over(base, masked(G, m))
    bevel(base, m, light=(255, 255, 240), dark=(20, 8, 0), depth=0.5)
    hi = Image.new('L', base.size, 0)
    ImageDraw.Draw(hi).ellipse(((cx - r * 0.45) * SS, (cy - r * 0.6) * SS, (cx + r * 0.05) * SS, (cy - r * 0.1) * SS), fill=255)
    H = Image.new('RGBA', base.size, (255, 255, 255, 0))
    H.putalpha(hi.point(lambda v: int(v * 0.8)))
    over(base, H)

def hex_pts(x0, y0, x1, y1, tip):
    cy = (y0 + y1) / 2
    return [(x0 + tip, y0), (x1 - tip, y0), (x1, cy), (x1 - tip, y1), (x0 + tip, y1), (x0, cy)]

def glyph_row(base, x, y, col, n=6, seed=2):
    rnd = random.Random(seed)
    L = Image.new('RGBA', base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(L)
    for i in range(n):
        gx = x + i * 5.2
        kind = rnd.randint(0, 3)
        w = int(0.8 * SS)
        d.line((gx * SS, (y - 3) * SS, gx * SS, (y + 3) * SS), fill=col, width=w)
        if kind == 0:
            d.line((gx * SS, (y - 3) * SS, (gx + 2.2) * SS, (y - 0.8) * SS), fill=col, width=w)
            d.line(((gx + 2.2) * SS, (y - 0.8) * SS, gx * SS, y * SS), fill=col, width=w)
        elif kind == 1:
            d.line(((gx - 2) * SS, (y - 1.6) * SS, (gx + 2) * SS, (y + 1.6) * SS), fill=col, width=w)
        elif kind == 2:
            d.line(((gx - 2) * SS, (y + 3) * SS, gx * SS, (y - 0.5) * SS), fill=col, width=w)
            d.line((gx * SS, (y - 0.5) * SS, (gx + 2) * SS, (y + 3) * SS), fill=col, width=w)
        else:
            d.line((gx * SS, (y - 3) * SS, (gx + 2.3) * SS, (y - 3) * SS), fill=col, width=w)
            d.line(((gx + 2.3) * SS, (y - 3) * SS, (gx + 2.3) * SS, y * SS), fill=col, width=w)
    over(base, L)
    add_glow(base, L, 1.1, 1.1)
    return base

def title(base, kind, lvl, cx):
    th = THEMES[kind]
    f1 = font('COPRGTB.TTF', 14.6)
    f2 = font('COPRGTB.TTF', 7.4)
    top, bottom = (255, 252, 240), tuple(int(c * 0.55 + 255 * 0.45) for c in th['glow2'])
    emboss_text(base, th['label'], f1, cx, M + 16.4, top=top, bottom=bottom, glow_color=th['glow2'], glow_r=1.3,
                glow_k=0.75 * lvl, spacing=0.9)
    emboss_text(base, th['sub'], f2, cx, M + 30.4, top=tuple(min(255, c + 20) for c in th['sub_col']),
                bottom=tuple(int(c * 0.72) for c in th['sub_col']), outline=(24, 10, 4), spacing=0.7)


def rune_layer(kind):
    """Only the glowing parts (rune line, emblem frame, runes) on transparent: the client pulses this over the plate."""
    th = THEMES[kind]
    size = (CW * SS, CH * SS)
    img = Image.new('RGBA', size, (0, 0, 0, 0))
    tip = 11.0
    lp = hex_pts(M + 3.6, M + 3.4, M + PW - 3.6, M + PH - 3.4, tip - 1.6)
    line = stroke_path(img, lp, 0.8, th['glow2'] + (255,))
    over(img, line)
    cx, cy = M + 26, M + 20
    hp = [(cx + 17.0 * math.cos(math.radians(a)), cy + 15.5 * math.sin(math.radians(a))) for a in range(0, 360, 60)]
    over(img, stroke_path(img, hp, 0.9, th['glow2'] + (255,)))
    glyph_row(img, M + PW - 31, M + 20, th['glow2'] + (255,), n=3, seed=(2 if kind == 'ember' else 5))
    add_glow(img, img.copy(), 1.4, 1.0)
    return img


def plate_c(kind, lvl=1.0, seed=5):
    """One plate at 8x. lvl scales the glows: ~0.8 idle, 1.4 hover, 0.5 pressed."""
    th = THEMES[kind]
    size = (CW * SS, CH * SS)
    img = Image.new('RGBA', size, (0, 0, 0, 0))
    tip = 11.0
    ox0, oy0, ox1, oy1 = M, M, M + PW, M + PH
    outer = poly_mask(size, hex_pts(ox0, oy0, ox1, oy1, tip))
    sh = ImageChops.offset(outer, 0, int(1.0 * SS)).filter(ImageFilter.GaussianBlur(1.6 * SS))
    S = Image.new('RGBA', size, (0, 0, 0, 0))
    S.putalpha(sh.point(lambda v: int(v * 0.9)))
    over(img, S)

    # steel hairline rim
    rim = vgrad(size, [(0, (170, 172, 180, 255)), (0.5, (84, 86, 94, 255)), (1, (40, 40, 46, 255))])
    over(img, masked(rim, outer))
    bevel(img, outer, light=(255, 255, 255), dark=(0, 0, 0), depth=0.9, a_light=170, a_dark=200)

    # obsidian
    om = poly_mask(size, hex_pts(ox0 + 1.4, oy0 + 1.4, ox1 - 1.4, oy1 - 1.4, tip - 0.4))
    ob = tint_noise((16, 16, 22), grain(size, 7, seed), size)
    ob.alpha_composite(vgrad(size, [(0, (255, 255, 255, 34)), (0.45, (0, 0, 0, 0)), (1, (0, 0, 0, 120))]))
    ob.alpha_composite(radial(size, M + 100, M + PH + 4, 130, 28, th['glow'] + (int(105 * lvl),), 1.3))
    ob.alpha_composite(radial(size, M + 26, M + 20, 44, 30, th['glow'] + (int(110 * lvl),), 1.5))
    yy, xx = np.mgrid[0:size[1], 0:size[0]].astype(np.float32)
    for off, wd, k in ((0.28, 0.035, 46), (0.34, 0.012, 60), (0.78, 0.05, 26)):
        band = np.exp(-(((xx / size[0]) - off - (yy / size[1]) * 0.18) / wd) ** 2) * k
        arr = np.zeros((size[1], size[0], 4), np.uint8)
        arr[:, :, :3] = 255
        arr[:, :, 3] = band.clip(0, 255).astype(np.uint8)
        ob.alpha_composite(Image.fromarray(arr, 'RGBA'))
    over(img, masked(ob, om))
    inner_shadow(img, om, 1.4, 220)

    # glowing rune line inset
    lp = hex_pts(ox0 + 3.6, oy0 + 3.4, ox1 - 3.6, oy1 - 3.4, tip - 1.6)
    line = stroke_path(img, lp, 0.8, th['glow2'] + (int(235 * min(1.0, lvl)),))
    over(img, line)
    add_glow(img, line, 1.5, 0.95 * lvl)
    for px, py in ((ox0 + tip + 1, oy0 + 3.4), (ox1 - tip - 1, oy0 + 3.4), (ox0 + tip + 1, oy1 - 3.4),
                   (ox1 - tip - 1, oy1 - 3.4)):
        rivet(img, px, py, 1.1, (255, 255, 255), (120, 124, 134))

    # crest in a hexagon frame
    cx, cy = M + 26, M + 20
    hp = [(cx + 17.0 * math.cos(math.radians(a)), cy + 15.5 * math.sin(math.radians(a))) for a in range(0, 360, 60)]
    emm = poly_mask(size, hp)
    over(img, masked(vgrad(size, [(0, (12, 8, 8, 255)), (1, tuple(int(c * 0.3) for c in th['panel']) + (255,))]), emm))
    over(img, masked(radial(size, cx, cy + 4, 16, 16, th['glow'] + (int(225 * lvl),), 1.2), emm))
    ln = stroke_path(img, hp, 0.9, th['glow2'] + (255,))
    over(img, ln)
    add_glow(img, ln, 1.4, 1.0 * lvl)
    inner_shadow(img, emm, 1.0, 200)
    if kind == 'ember':
        crest_ember(img, cx, cy - 1.4, 1.0, th['glow'])
    else:
        crest_inferno(img, cx, cy + 0.6, 0.94, th['glow'])

    title(img, kind, lvl, M + 100)
    glyph_row(img, M + PW - 31, M + 20, th['glow2'] + (int(200 * min(lvl, 1.3)),), n=3, seed=(2 if kind == 'ember' else 5))
    return img


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


STATE_LEVELS = (0.8, 1.4, 0.5)   # idle, hover, pressed


def cells():
    """[column][row] -> 8x RGBA cell; columns Ember, Inferno; rows idle, hover, pressed, rune layer."""
    out = []
    for kind in ('ember', 'inferno'):
        out.append([plate_c(kind, lvl) for lvl in STATE_LEVELS] + [rune_layer(kind)])
    return out


def atlas(master_cells, scale):
    cw, ch = int(CW * scale), int(CH * scale)
    sheet = Image.new('RGBA', (cw * 2, ch * 4), (0, 0, 0, 0))
    for col, column in enumerate(master_cells):
        for row, cell in enumerate(column):
            sheet.paste(downscale(cell, scale), (col * cw, row * ch))
    return sheet


def main():
    repo = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
    data = os.path.join(repo, 'src', 'bin', 'Data')
    master = cells()
    png_to_ozt(atlas(master, 1), os.path.join(data, 'Interface', 'server_plates.OZT'))
    atlas(master, 2).save(os.path.join(data, 'Interface@2x', 'server_plates.png'))
    atlas(master, 4).save(os.path.join(data, 'Interface@4x', 'server_plates.png'))
    print('wrote the server plate atlas')


if __name__ == '__main__':
    main()
