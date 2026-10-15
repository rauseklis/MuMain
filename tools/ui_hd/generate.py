"""Builds the HD tiers for the core interface set.

  python tools/ui_hd/generate.py            enlarge every core texture to the 4x master, then derive 2x from it
  python tools/ui_hd/generate.py --only newui_item_back01.OZT

Masters are Data/Interface@4x/<same path>.png; Data/Interface@2x is always reduced from them.
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

from core_set import CORE_SET
from mu_image import read_texture
from upscale import add_grain, downscale_rgba, upscale_rgba

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA_DIR = os.path.join(REPO_ROOT, "src", "bin", "Data")
MASTER_SCALE = 4
TIERS = (4, 2)
GRAIN_TEXTURES = {"newui_msgbox_back.OZJ"}


def tier_path(relative, scale):
    stem = os.path.splitext(relative)[0]
    return os.path.join(DATA_DIR, f"Interface@{scale}x", stem + ".png")


def source_path(relative):
    return os.path.join(DATA_DIR, "Interface", relative)


def build_master(relative):
    original, kind = read_texture(source_path(relative))
    hd = upscale_rgba(original, MASTER_SCALE, jpeg_source=(kind == "ozj"))
    if relative in GRAIN_TEXTURES:
        hd = add_grain(hd, MASTER_SCALE)
    return hd


def save_png(path, rgba):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    Image.fromarray(rgba).save(path, optimize=True)


def write_tiers(relative, master):
    for scale in TIERS:
        reduced = master if scale == MASTER_SCALE else downscale_rgba(master, MASTER_SCALE // scale)
        save_png(tier_path(relative, scale), reduced)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--only")
    args = parser.parse_args()
    for relative in CORE_SET:
        if args.only and relative != args.only:
            continue
        write_tiers(relative, build_master(relative))
        print("ok", relative)


if __name__ == "__main__":
    sys.exit(main())
