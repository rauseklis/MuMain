"""Cuts the armour out of the worn-gear slot picture of the inventory, for the equipment row of the reset NPC window.

  python tools/ui_hd/reset_armour.py

The slot picture (Interface@4x/newui_item_upper.png) is the armour inside a dark rounded frame. The dark background around
the armour is removed from the edges inwards, so only the armour itself stays, on a transparent square. Writes
src/bin/Data/Interface/newui_reset_armour.OZT and its Interface@4x and Interface@2x tiers.
"""
import os
from collections import deque

import numpy as np
from PIL import Image, ImageFilter

import generate
from mu_image import write_ozt

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, "..", "..", "src", "bin", "Data", "Interface@4x", "newui_item_upper.png")
RELATIVE = "newui_reset_armour.OZT"
INSET = 28
BACKGROUND_LUMA = 62
LIT_LUMA = 80
CLOSE = 25
OPEN = 17
SIDE = 160
MIN_PIECE = 400


def background_mask(luma):
    """Dark pixels connected to the border of the picture."""
    height, width = luma.shape
    dark = luma < BACKGROUND_LUMA
    seen = np.zeros_like(dark)
    queue = deque()
    for x in range(width):
        for y in (0, height - 1):
            if dark[y, x] and not seen[y, x]:
                seen[y, x] = True
                queue.append((y, x))
    for y in range(height):
        for x in (0, width - 1):
            if dark[y, x] and not seen[y, x]:
                seen[y, x] = True
                queue.append((y, x))
    while queue:
        y, x = queue.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            ny, nx = y + dy, x + dx
            if 0 <= ny < height and 0 <= nx < width and dark[ny, nx] and not seen[ny, nx]:
                seen[ny, nx] = True
                queue.append((ny, nx))
    return seen


def largest_piece(solid):
    """The True pixels without the small pieces (leftovers of the frame)."""
    height, width = solid.shape
    seen = np.zeros_like(solid)
    best = []
    for y0 in range(height):
        for x0 in range(width):
            if not solid[y0, x0] or seen[y0, x0]:
                continue
            piece = []
            queue = deque([(y0, x0)])
            seen[y0, x0] = True
            while queue:
                y, x = queue.popleft()
                piece.append((y, x))
                for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < height and 0 <= nx < width and solid[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        queue.append((ny, nx))
            if len(piece) >= MIN_PIECE:
                best.extend(piece)
    keep = np.zeros_like(solid)
    for y, x in best:
        keep[y, x] = True
    return keep


def main():
    image = Image.open(SOURCE).convert("RGBA")
    inner = image.crop((INSET, INSET, image.width - INSET, image.height - INSET))
    rgb = np.array(inner)[:, :, :3].astype(np.float32)
    luma = rgb @ np.array([0.299, 0.587, 0.114], dtype=np.float32)
    outside = background_mask(luma)

    # The silhouette of the armour: its lit edges, closed up into one shape and filled.
    rgb = np.array(inner)[:, :, :3].astype(np.float32)
    luma = rgb @ np.array([0.299, 0.587, 0.114], dtype=np.float32)
    lit = Image.fromarray(np.where(luma > LIT_LUMA, 255, 0).astype(np.uint8), "L")
    closed = lit.filter(ImageFilter.MaxFilter(CLOSE)).filter(ImageFilter.MinFilter(CLOSE))
    closed = closed.filter(ImageFilter.MinFilter(OPEN)).filter(ImageFilter.MaxFilter(OPEN))
    outside = background_mask(np.where(np.array(closed) > 0, 255.0, 0.0))
    alpha = Image.fromarray(np.where(outside, 0, 255).astype(np.uint8), "L")
    alpha = alpha.filter(ImageFilter.MinFilter(3)).filter(ImageFilter.GaussianBlur(2.0))
    lifted = (255.0 * np.power(rgb / 255.0, 0.62)).astype(np.uint8)
    inner = Image.fromarray(lifted, "RGB").convert("RGBA")
    inner.putalpha(alpha)

    box = inner.getbbox()
    armour = inner.crop(box)
    scale = (SIDE - 8) / max(armour.size)
    armour = armour.resize((max(1, round(armour.width * scale)), max(1, round(armour.height * scale))), Image.LANCZOS)
    sheet = Image.new("RGBA", (SIDE, SIDE), (0, 0, 0, 0))
    sheet.paste(armour, ((SIDE - armour.width) // 2, (SIDE - armour.height) // 2))

    master = np.array(sheet)
    base = np.array(sheet.resize((SIDE // 4, SIDE // 4), Image.LANCZOS))
    write_ozt(generate.source_path(RELATIVE), base)
    generate.write_tiers(RELATIVE, master, (4, 2))
    print("ok", master.shape)


if __name__ == "__main__":
    main()
