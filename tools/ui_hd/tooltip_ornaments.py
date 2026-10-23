"""Builds the gold ornament atlas of the buff tooltip from the Auction House frame art.

  python tools/ui_hd/tooltip_ornaments.py

Writes src/bin/Data/Interface/BuffDetails/tooltip_ornaments.OZT (1x, the original the loader looks for) and the
Interface@4x and Interface@2x tiers. Layout in logical pixels (128 x 32): the corner ornament (24 x 24) at 0,0 and the
gold rule (96 x 4) at 28,0, and two border strips cut from the straight part of the corner art so the edges match the
corners exactly: the top one (4 x 2.75) at 28,6 and the left one (2.75 x 4) at 28,10. The corner is the top-left one; the
game mirrors it for the other three and stretches the strips along the edges.
"""
import os

import numpy as np
from PIL import Image

from mu_image import write_ozt
from upscale import downscale_rgba

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(REPO_ROOT, "src", "bin", "Data")
RELATIVE = "BuffDetails/tooltip_ornaments"
MASTER_SCALE = 4
ATLAS_SIZE = (128, 32)
CORNER_SIZE = 24
CORNER_ORIGIN = (0, 0)
RULE_SIZE = (96, 4)
RULE_ORIGIN = (28, 0)
STRIP_SOURCE_BAND = 22
STRIP_SOURCE_RUN = (156, 188)
STRIP_SIZE = (16, 11)
TOP_STRIP_ORIGIN = (112, 24)
LEFT_STRIP_ORIGIN = (112, 40)
RULE_SOURCE_ROWS = (148, 157)
RULE_SOURCE_WIDTH = 390


def load(name):
    return Image.open(os.path.join(DATA, "Interface@2x", name + ".png")).convert("RGBA")


def resize_premultiplied(image, size):
    data = np.asarray(image, np.float32)
    alpha = data[..., 3:4] / 255.0
    premult = Image.fromarray(np.concatenate([data[..., :3] * alpha, data[..., 3:4]], axis=2).astype(np.float32).clip(0, 255).astype(np.uint8), "RGBA")
    resized = np.asarray(premult.resize(size, Image.LANCZOS), np.float32)
    a = resized[..., 3:4] / 255.0
    color = np.where(a > 1e-4, resized[..., :3] / np.maximum(a, 1e-4), 0.0)
    return Image.fromarray(np.clip(np.concatenate([color, resized[..., 3:4]], axis=2), 0, 255).astype(np.uint8), "RGBA")


def save(path, image):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    image.save(path, optimize=True)


def build_master():
    scale = MASTER_SCALE
    atlas = Image.new("RGBA", (ATLAS_SIZE[0] * scale, ATLAS_SIZE[1] * scale), (0, 0, 0, 0))
    corner = resize_premultiplied(load("newui_auction_frame_corner"), (CORNER_SIZE * scale, CORNER_SIZE * scale))
    atlas.alpha_composite(corner, (CORNER_ORIGIN[0] * scale, CORNER_ORIGIN[1] * scale))
    rule = load("icon_Rank_table").crop((0, RULE_SOURCE_ROWS[0], RULE_SOURCE_WIDTH, RULE_SOURCE_ROWS[1]))
    rule = resize_premultiplied(rule, (RULE_SIZE[0] * scale, RULE_SIZE[1] * scale))
    atlas.alpha_composite(rule, (RULE_ORIGIN[0] * scale, RULE_ORIGIN[1] * scale))
    corner_source = load("newui_auction_frame_corner")
    run = STRIP_SOURCE_RUN
    top = corner_source.crop((run[0], 0, run[1], STRIP_SOURCE_BAND))
    left = corner_source.crop((0, run[0], STRIP_SOURCE_BAND, run[1]))
    atlas.alpha_composite(resize_premultiplied(top, STRIP_SIZE), TOP_STRIP_ORIGIN)
    atlas.alpha_composite(resize_premultiplied(left, STRIP_SIZE[::-1]), LEFT_STRIP_ORIGIN)
    return np.asarray(atlas)


def main():
    master = build_master()
    save(os.path.join(DATA, "Interface@4x", RELATIVE + ".png"), Image.fromarray(master))
    save(os.path.join(DATA, "Interface@2x", RELATIVE + ".png"), Image.fromarray(downscale_rgba(master, 2)))
    original = os.path.join(DATA, "Interface", *RELATIVE.split("/"))
    os.makedirs(os.path.dirname(original), exist_ok=True)
    write_ozt(original + ".OZT", downscale_rgba(master, MASTER_SCALE))
    print("ok")


if __name__ == "__main__":
    main()
