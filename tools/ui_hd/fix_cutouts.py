"""Regenerates every HD model texture whose original has transparent cut-outs, keeping the colour under the transparency.

  python tools/ui_hd/fix_cutouts.py

The game draws some cut-out textures (for example the Dark Lord cape cloth) without testing alpha, so the colour that the
original stores under fully transparent pixels is visible. A premultiplied enlargement turns that colour black. This script
redoes those files with a straight (non-premultiplied) enlargement.
"""
import glob
import os
import sys

import numpy as np
from PIL import Image

from generate import DATA_DIR
from mu_image import read_texture
from upscale import upscale_rgba_straight

SKIPPED_FOLDERS = ("Interface",)


def source_for(folder, hd_name):
    """The original .ozj/.ozt file a HD file stands for, or None. Handles the plain and the extension-qualified names."""
    base = hd_name[:-4]
    lower = base.lower()
    wanted = []
    if lower.endswith(".jpg"):
        wanted = [base[:-4] + ".ozj"]
    elif lower.endswith(".tga"):
        wanted = [base[:-4] + ".ozt"]
    else:
        wanted = [base + ".ozt", base + ".ozj"]
    present = {name.lower(): name for name in os.listdir(os.path.join(DATA_DIR, folder))}
    for name in wanted:
        if name.lower() in present:
            return os.path.join(DATA_DIR, folder, present[name.lower()])
    return None


def main():
    fixed = 0
    for hd_dir in sorted(glob.glob(os.path.join(DATA_DIR, "*@[24]x"))):
        folder, scale_text = os.path.basename(hd_dir).split("@")
        if folder in SKIPPED_FOLDERS:
            continue
        scale = int(scale_text[:-1])
        for png in glob.glob(os.path.join(hd_dir, "*.png")):
            source = source_for(folder, os.path.basename(png))
            if source is None or not source.lower().endswith(".ozt"):
                continue
            rgba, _ = read_texture(source)
            if not (rgba[..., 3] < 255).any():
                continue
            Image.fromarray(upscale_rgba_straight(rgba, scale)).save(png, optimize=True)
            fixed += 1
    print(f"{fixed} cut-out textures regenerated")
    return 0


if __name__ == "__main__":
    sys.exit(main())
