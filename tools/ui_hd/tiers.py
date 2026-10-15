"""Rebuilds every lower tier (Interface@2x) from the 4x masters in Interface@4x.

  python tools/ui_hd/tiers.py
Use it after replacing a master PNG by hand.
"""
import os
import sys

import numpy as np
from PIL import Image

from generate import DATA_DIR, MASTER_SCALE, REST_TIERS as TIERS, save_png
from upscale import downscale_rgba

MASTER_DIR = os.path.join(DATA_DIR, f"Interface@{MASTER_SCALE}x")


def main():
    count = 0
    for folder, _, files in os.walk(MASTER_DIR):
        for name in files:
            if not name.lower().endswith(".png"):
                continue
            master_path = os.path.join(folder, name)
            master = np.asarray(Image.open(master_path).convert("RGBA"))
            relative = os.path.relpath(master_path, MASTER_DIR)
            for scale in TIERS:
                if scale == MASTER_SCALE:
                    continue
                reduced = downscale_rgba(master, MASTER_SCALE // scale)
                save_png(os.path.join(DATA_DIR, f"Interface@{scale}x", relative), reduced)
            count += 1
    print(f"{count} masters reduced")


if __name__ == "__main__":
    sys.exit(main())
