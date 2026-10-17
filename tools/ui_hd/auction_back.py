"""Draws the Auction House window background (600 x 429 logical pixels).

  python tools/ui_hd/auction_back.py

Writes newui_auction_back:
  src/bin/Data/Interface/newui_auction_back.OZT      original size (600 x 429)
  src/bin/Data/Interface@4x/newui_auction_back.png   4x master (2400 x 1716)
  src/bin/Data/Interface@2x/newui_auction_back.png   2x tier (1200 x 858)

Dark blue-black flagstones with bevelled joints, a warm gold glow behind the title band, a soft vignette, a faint
diamond lattice and a large engraved coin-and-laurel emblem in the middle. It stays low contrast on purpose: the tabs,
the table and the wallet panels are drawn on top and must stay readable. The frame pieces cover the outer edge.
"""
import os

import numpy as np
from PIL import Image, ImageDraw

from mu_image import write_ozt
from upscale import gaussian_blur

WIDTH, HEIGHT = 600, 429
SCALE = 4
W, H = WIDTH * SCALE, HEIGHT * SCALE

STONE_DARK = np.array((14, 17, 24), np.float32)
STONE_LIGHT = np.array((34, 38, 48), np.float32)
GOLD = np.array((224, 168, 62), np.float32)
GLOW = np.array((150, 96, 26), np.float32)

FLAG_W, FLAG_H = 75, 53.625  # logical size of one flagstone (8 x 8 stones)


def blurred_noise(rng, sigma, shape=(H, W)):
    noise = rng.normal(0.0, 1.0, shape).astype(np.float32)
    noise = gaussian_blur(noise, sigma)
    return noise / max(float(noise.std()), 1e-6)


def stone_layer(rng):
    """Flagstone base: per-stone tone, fine grain, mottling, bevelled joints."""
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    stone_w, stone_h = FLAG_W * SCALE, FLAG_H * SCALE
    row = np.floor(yy / stone_h)
    offset = (row % 2) * stone_w * 0.5  # running bond
    col = np.floor((xx + offset) / stone_w)
    local_x = (xx + offset) - col * stone_w
    local_y = yy - row * stone_h

    tone_rng = np.random.default_rng(11)
    tones = tone_rng.uniform(0.0, 1.0, (64, 64)).astype(np.float32)
    tone = tones[(row.astype(int) % 64), (col.astype(int) % 64)]

    base = STONE_DARK[None, None, :] + (STONE_LIGHT - STONE_DARK)[None, None, :] * (0.25 + 0.5 * tone)[..., None]
    mottle = blurred_noise(rng, 26 * SCALE / 4)
    grain = blurred_noise(rng, 0.9)
    base = base * (1.0 + 0.10 * mottle[..., None] + 0.05 * grain[..., None])

    # joints: dark groove with a lit lower-left lip and a shadowed upper-right edge
    edge = np.minimum.reduce([local_x, stone_w - local_x, local_y, stone_h - local_y])
    groove = np.clip(1.0 - edge / (1.6 * SCALE), 0.0, 1.0) ** 1.5
    bevel = np.clip(1.0 - edge / (5.0 * SCALE), 0.0, 1.0)
    light_side = (np.clip(1.0 - local_x / (4 * SCALE), 0, 1) + np.clip(1.0 - local_y / (4 * SCALE), 0, 1)) * 0.5
    shade_side = (np.clip(1.0 - (stone_w - local_x) / (4 * SCALE), 0, 1) + np.clip(1.0 - (stone_h - local_y) / (4 * SCALE), 0, 1)) * 0.5
    base = base * (1.0 - 0.75 * groove[..., None])
    base = base + 14.0 * (light_side * bevel)[..., None] - 10.0 * (shade_side * bevel)[..., None]
    return base


def lattice_layer():
    """A faint diamond lattice, drawn as thin lines, returned as an additive intensity map (0..1)."""
    img = Image.new("L", (W, H), 0)
    draw = ImageDraw.Draw(img)
    step = 30 * SCALE
    width = max(int(0.6 * SCALE), 1)
    for k in range(-H // step - 1, W // step + H // step + 2):
        x0 = k * step
        draw.line([(x0, 0), (x0 + H, H)], fill=255, width=width)
        draw.line([(x0 + H, 0), (x0, H)], fill=255, width=width)
    arr = np.asarray(img, np.float32) / 255.0
    return gaussian_blur(arr, 0.5 * SCALE / 4)


def emblem_layer():
    """Engraved coin ringed by laurel leaves, centred in the body. Intensity map (0..1)."""
    sup = 2
    size = (W // 2 * sup // 2, H // 2 * sup // 2)  # drawn at half master resolution, scaled up later
    cx, cy = size[0] / 2, size[1] * 0.56
    unit = size[0] / WIDTH
    img = Image.new("L", size, 0)
    d = ImageDraw.Draw(img)
    for radius, width in ((118, 1.6), (112, 0.8), (84, 1.4), (78, 0.8), (50, 1.2)):
        d.ellipse([cx - radius * unit, cy - radius * unit, cx + radius * unit, cy + radius * unit], outline=255, width=max(int(width * unit), 1))
    # laurel leaves around the middle ring
    rng = np.random.default_rng(3)
    for side in (-1, 1):
        for i in range(11):
            ang = np.radians(-105 + i * 17.5) * side + (np.pi if side < 0 else 0.0)
            ang = np.radians(95 + i * 15) if side > 0 else np.radians(85 - i * 15)
            r = 99 * unit
            px, py = cx + r * np.cos(ang), cy + r * np.sin(ang)
            tang = ang + np.pi / 2
            ln, wd = 15 * unit, 5 * unit
            pts = []
            for t in np.linspace(0, 2 * np.pi, 16):
                lx, ly = np.cos(t) * ln / 2, np.sin(t) * wd / 2
                rot = tang + side * 0.5
                pts.append((px + lx * np.cos(rot) - ly * np.sin(rot), py + lx * np.sin(rot) + ly * np.cos(rot)))
            d.polygon(pts, outline=255)
    # centre: a crown-like glyph of three points over a bar, and spokes
    for i in range(24):
        ang = i * np.pi / 12
        d.line([(cx + 52 * unit * np.cos(ang), cy + 52 * unit * np.sin(ang)),
                (cx + 76 * unit * np.cos(ang), cy + 76 * unit * np.sin(ang))], fill=170, width=max(int(0.7 * unit), 1))
    crown = [(-26, 14), (-26, -12), (-13, 2), (0, -20), (13, 2), (26, -12), (26, 14)]
    d.line([(cx + x * unit, cy + y * unit) for x, y in crown] + [(cx - 26 * unit, cy + 14 * unit)], fill=255, width=max(int(1.6 * unit), 1))
    d.line([(cx - 26 * unit, cy + 22 * unit), (cx + 26 * unit, cy + 22 * unit)], fill=255, width=max(int(2.2 * unit), 1))
    for x in (-26, 0, 26):
        r = 3.2 * unit
        d.ellipse([cx + x * unit - r, cy + (-14 if x else -24) * unit - r, cx + x * unit + r, cy + (-14 if x else -24) * unit + r], outline=255, width=max(int(unit), 1))
    img = img.resize((W, H), Image.LANCZOS)
    arr = np.asarray(img, np.float32) / 255.0
    return gaussian_blur(arr, 0.45 * SCALE / 4)


def render():
    rng = np.random.default_rng(5)
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    u, v = xx / W, yy / H

    img = stone_layer(rng)

    # darken the body overall so UI text stays readable
    img *= 0.78

    # vignette: the corners and edges sink into shadow
    d = np.sqrt(((u - 0.5) / 0.62) ** 2 + ((v - 0.52) / 0.68) ** 2)
    vignette = np.clip(1.0 - 0.62 * np.clip(d, 0, 1.4) ** 2.2, 0.28, 1.0)
    img *= vignette[..., None]

    # warm glow behind the title band, fading down the window
    glow = np.exp(-(((u - 0.5) / 0.46) ** 2 + (v / 0.28) ** 2)) * 0.9
    img += glow[..., None] * GLOW[None, None, :] * 0.55

    # cool light pooling at the bottom for depth
    pool = np.exp(-(((u - 0.5) / 0.55) ** 2 + ((v - 1.0) / 0.22) ** 2)) * 0.5
    img += pool[..., None] * np.array((16, 26, 44), np.float32)[None, None, :]

    # lattice + emblem, in gold, low strength
    lat = lattice_layer() * (0.35 + 0.65 * np.clip(v * 1.4, 0, 1)) * vignette
    img += lat[..., None] * GOLD[None, None, :] * 0.050
    emb = emblem_layer()
    img += emb[..., None] * GOLD[None, None, :] * 0.115

    # thin gold inset line hugging the frame, a few logical pixels inside the edge
    inset = 26 * SCALE
    line_w = 0.8 * SCALE
    dist = np.minimum.reduce([xx - inset, W - 1 - xx - inset, yy - inset, H - 1 - yy - inset])
    inset_line = np.exp(-(dist / line_w) ** 2) * (dist > -line_w * 3)
    inset_dark = np.exp(-((dist - 3 * SCALE) / (1.2 * SCALE)) ** 2) * 0.5
    img += inset_line[..., None] * GOLD[None, None, :] * 0.30
    img -= inset_dark[..., None] * 14.0

    # corner studs on the inset line
    for (px, py) in ((inset, inset), (W - 1 - inset, inset), (inset, H - 1 - inset), (W - 1 - inset, H - 1 - inset)):
        r2 = ((xx - px) ** 2 + (yy - py) ** 2) / (3.4 * SCALE) ** 2
        img += (np.exp(-r2 * 2.0) * 0.9)[..., None] * GOLD[None, None, :] * 0.55

    # final fine grain so gradients do not band
    img += rng.normal(0.0, 1.1, (H, W, 1)).astype(np.float32)
    return np.clip(img, 0, 255).astype(np.uint8)


def main():
    master_rgb = render()
    master = Image.fromarray(master_rgb, "RGB").convert("RGBA")
    data = os.path.join(os.path.dirname(__file__), "..", "..", "src", "bin", "Data")
    original = master.resize((WIDTH, HEIGHT), Image.LANCZOS)
    write_ozt(os.path.join(data, "Interface", "newui_auction_back.OZT"), np.asarray(original))
    master.save(os.path.join(data, "Interface@4x", "newui_auction_back.png"), optimize=True)
    master.resize((WIDTH * 2, HEIGHT * 2), Image.LANCZOS).save(os.path.join(data, "Interface@2x", "newui_auction_back.png"), optimize=True)
    preview = os.environ.get("AUCTION_BACK_PREVIEW")
    if preview:
        original.save(preview)
    print("wrote newui_auction_back")


if __name__ == "__main__":
    main()
