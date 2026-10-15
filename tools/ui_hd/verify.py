"""Checks every HD texture against its original and lists the ones that drifted.

  python tools/ui_hd/verify.py [--mae 8] [--ssim 0.9] [--alpha 8]
For each tier the HD PNG is reduced back to the original size and compared: mean absolute error and SSIM on
premultiplied colour, and mean alpha error. Exit status is 1 when a texture is out of tolerance or has a wrong size.
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

import gauges
import gm_icon
import reframe
import retext
from compare import premultiplied, ssim
from generate import DATA_DIR, tier_path
from mu_image import read_texture
from survey import survey
from upscale import downscale_rgba

TIERS = (4, 2)
DEFAULT_MAX_MAE = 8.0
DEFAULT_MIN_SSIM = 0.90
DEFAULT_MAX_ALPHA_ERROR = 10.0


REDRAWN = {name.lower() for name in set(retext.SPECS) | set(reframe.SPECS) | set(reframe.SLOT_STRIPS) | set(reframe.OUTER_CORNERS) | gauges.SPECS | gauges.GEM_SPECS | {gm_icon.ATLAS}}


def check(relative, scale):
    path = tier_path(relative, scale)
    if not os.path.exists(path):
        return None
    original, _ = read_texture(os.path.join(DATA_DIR, "Interface", relative))
    hd = np.asarray(Image.open(path).convert("RGBA"))
    height, width = original.shape[:2]
    if hd.shape[:2] != (height * scale, width * scale):
        return ("size", hd.shape[1], hd.shape[0], width, height)
    reduced = downscale_rgba(hd, scale)
    a, b = premultiplied(original), premultiplied(reduced)
    alpha_error = float(np.mean(np.abs(original[..., 3].astype(int) - reduced[..., 3].astype(int))))
    return ("ok", float(np.mean(np.abs(a - b))), ssim(a, b), alpha_error)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mae", type=float, default=DEFAULT_MAX_MAE)
    parser.add_argument("--ssim", type=float, default=DEFAULT_MIN_SSIM)
    parser.add_argument("--alpha", type=float, default=DEFAULT_MAX_ALPHA_ERROR)
    args = parser.parse_args()
    failures, checked = [], 0
    for relative, *_ in survey():
        if relative.lower() in REDRAWN:
            continue
        for scale in TIERS:
            result = check(relative, scale)
            if result is None:
                continue
            checked += 1
            if result[0] == "size":
                failures.append((relative, scale, f"size {result[1]}x{result[2]} for original {result[3]}x{result[4]}"))
            elif result[1] > args.mae or result[2] < args.ssim or result[3] > args.alpha:
                failures.append((relative, scale, f"MAE {result[1]:.2f} SSIM {result[2]:.3f} alphaMAE {result[3]:.2f}"))
    print(f"{checked} HD textures checked, {len(failures)} out of tolerance")
    for relative, scale, text in failures:
        print(f"  {scale}x {relative}: {text}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
