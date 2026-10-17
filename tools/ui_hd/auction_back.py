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

STONE_DARK = np.array((26, 26, 30), np.float32)
STONE_LIGHT = np.array((74, 70, 70), np.float32)
GOLD = np.array((224, 168, 62), np.float32)
GLOW = np.array((150, 96, 26), np.float32)

FLAG_W, FLAG_H = 75, 53.625  # logical size of one flagstone (8 x 8 stones)


def blurred_noise(rng, sigma, shape=(H, W)):
    noise = rng.normal(0.0, 1.0, shape).astype(np.float32)
    noise = gaussian_blur(noise, sigma)
    return noise / max(float(noise.std()), 1e-6)


def stone_layer(rng):
    """Rough hand-laid masonry: rows of uneven height, blocks of uneven width, wobbly worn joints, uneven tone."""
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    # domain warp so every joint wanders and no edge is a ruler line
    wx = blurred_noise(rng, 9 * SCALE) * 3.2 * SCALE + blurred_noise(rng, 2.5 * SCALE) * 1.1 * SCALE
    wy = blurred_noise(rng, 9 * SCALE) * 3.2 * SCALE + blurred_noise(rng, 2.5 * SCALE) * 1.1 * SCALE
    px, py = xx + wx, yy + wy

    layout = np.random.default_rng(17)
    row_edges = [0.0]
    while row_edges[-1] < H + 80 * SCALE:
        row_edges.append(row_edges[-1] + layout.uniform(38, 72) * SCALE)
    row_edges = np.array(row_edges, np.float32)
    row = np.clip(np.searchsorted(row_edges, py, side="right") - 1, 0, len(row_edges) - 2)

    tone = np.zeros((H, W), np.float32)
    edge = np.full((H, W), 1e9, np.float32)
    for r in range(len(row_edges) - 1):
        mask = row == r
        if not mask.any():
            continue
        cuts = [-layout.uniform(0, 60) * SCALE]
        while cuts[-1] < W + 100 * SCALE:
            cuts.append(cuts[-1] + layout.uniform(44, 118) * SCALE)
        cuts = np.array(cuts, np.float32)
        col = np.clip(np.searchsorted(cuts, px, side="right") - 1, 0, len(cuts) - 2)
        tones = layout.uniform(0.0, 1.0, len(cuts)).astype(np.float32)
        left = cuts[col]
        right = cuts[col + 1]
        d = np.minimum.reduce([px - left, right - px, py - row_edges[r], row_edges[r + 1] - py])
        tone = np.where(mask, tones[col], tone)
        edge = np.where(mask, d, edge)

    base = STONE_DARK[None, None, :] + (STONE_LIGHT - STONE_DARK)[None, None, :] * (0.15 + 0.7 * tone)[..., None]
    mottle = blurred_noise(rng, 14 * SCALE)
    grain = blurred_noise(rng, 0.9)
    coarse = blurred_noise(rng, 3.0 * SCALE)
    base = base * (1.0 + 0.16 * mottle[..., None] + 0.07 * grain[..., None] + 0.06 * coarse[..., None])

    # worn, rounded joints: a broad soft shadow, a ragged dark core, and eroded (noisy) edges
    ragged = edge + blurred_noise(rng, 2.2 * SCALE) * 0.55 * SCALE
    core = np.clip(1.0 - ragged / (1.5 * SCALE), 0.0, 1.0) ** 1.2
    soft = np.clip(1.0 - ragged / (7.0 * SCALE), 0.0, 1.0) ** 2
    base = base * (1.0 - 0.62 * core[..., None] - 0.30 * soft[..., None])
    # blocks catch light on their upper-left shoulder
    shoulder = np.clip(1.0 - ragged / (6.0 * SCALE), 0.0, 1.0) * np.clip(0.5 + 0.5 * (blurred_noise(rng, 5 * SCALE)), 0, 1)
    base = base + 9.0 * shoulder[..., None]
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
    vignette = np.clip(1.0 - 0.55 * np.clip(d, 0, 1.4) ** 2.2, 0.35, 1.0)
    img *= vignette[..., None]

    # warm glow behind the title band, fading down the window
    glow = np.exp(-(((u - 0.5) / 0.46) ** 2 + (v / 0.28) ** 2)) * 0.9
    img += glow[..., None] * GLOW[None, None, :] * 0.55

    # cool light pooling at the bottom for depth
    pool = np.exp(-(((u - 0.5) / 0.55) ** 2 + ((v - 1.0) / 0.22) ** 2)) * 0.5
    img += pool[..., None] * np.array((16, 26, 44), np.float32)[None, None, :]

    # rich colour wash: ember gold at the top, rose bronze, then deep teal-steel at the bottom, swept on a diagonal
    t = np.clip(v * 0.85 + (u - 0.5) * 0.22, 0.0, 1.0)
    stops = [(0.0, (232, 150, 40)), (0.28, (196, 82, 56)), (0.55, (92, 60, 120)), (0.8, (36, 88, 128)), (1.0, (24, 120, 132))]
    wash = np.zeros((H, W, 3), np.float32)
    for (t0, c0), (t1, c1) in zip(stops[:-1], stops[1:]):
        m = np.clip((t - t0) / (t1 - t0), 0.0, 1.0)
        seg = np.array(c0, np.float32) * (1 - m[..., None]) + np.array(c1, np.float32) * m[..., None]
        sel = ((t >= t0) & (t <= t1))[..., None]
        wash = np.where(sel, seg, wash)
    img = img * 0.62 + img * (wash / 255.0) * 0.85
    img += (np.exp(-(((u - 0.5) / 0.5) ** 2 + (v / 0.2) ** 2)) * 0.7)[..., None] * np.array((190, 120, 30), np.float32)[None, None, :] * 0.5

    # age: coarse pitting, grime pooled in the joints, rain streaks, hairline cracks and chipped corners
    pits = blurred_noise(rng, 1.6)
    img *= (1.0 - 0.16 * np.clip(pits - 0.9, 0, 2.0))[..., None]
    coarse = blurred_noise(rng, 7 * SCALE)
    img *= (1.0 + 0.10 * coarse)[..., None]
    streak_rng = np.random.default_rng(21)
    line = streak_rng.normal(0, 1, W).astype(np.float32)
    kernel = np.exp(-0.5 * (np.arange(-120, 121) / (7 * SCALE)) ** 2).astype(np.float32)
    line = np.convolve(line, kernel / kernel.sum(), mode="same")
    line /= max(float(line.std()), 1e-6)
    streaks = np.repeat(line[None, :], H, axis=0)
    fall = np.clip(1.0 - v * 1.2, 0.0, 1.0)
    img *= (1.0 - 0.07 * np.clip(streaks, 0, 2.5) * fall)[..., None]
    crack_img = Image.new("L", (W, H), 0)
    cd = ImageDraw.Draw(crack_img)
    crng = np.random.default_rng(8)
    for _ in range(26):
        x = crng.uniform(0, W)
        y = crng.uniform(0, H)
        ang = crng.uniform(0, 2 * np.pi)
        pts = [(x, y)]
        for _seg in range(int(crng.integers(5, 14))):
            ang += crng.normal(0, 0.22)
            step = crng.uniform(2.0, 5.5) * SCALE
            x += np.cos(ang) * step
            y += np.sin(ang) * step
            pts.append((x, y))
        cd.line(pts, fill=255, width=max(int(0.35 * SCALE), 1))
    cracks = gaussian_blur(np.asarray(crack_img, np.float32) / 255.0, 0.35 * SCALE)
    img *= (1.0 - 0.62 * np.clip(cracks * 1.6, 0, 1))[..., None]
    img += (np.roll(cracks, (SCALE, SCALE), axis=(0, 1)) * 12.0)[..., None]
    img *= np.array((1.0, 0.98, 0.94), np.float32)[None, None, :]

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
