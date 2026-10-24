"""Builds the fruit icon of the reset NPC window from the 32 x 32 picture in tools/ui_hd/art.

  python tools/ui_hd/reset_fruit.py

Writes src/bin/Data/Interface/newui_reset_fruit.OZT (the picture itself, the original the loader looks for) and its
Interface@4x and Interface@2x tiers, enlarged with Real-ESRGAN (see esrgan.py).
"""
import os

import numpy as np
from PIL import Image

import esrgan
import generate
from mu_image import write_ozt

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, "art", "reset_fruit_32.png")
RELATIVE = "newui_reset_fruit.OZT"
MODEL = "realesrgan-x4plus"


def main():
    rgba = np.array(Image.open(SOURCE).convert("RGBA"))
    write_ozt(generate.source_path(RELATIVE), rgba)
    master = esrgan.enlarge_many({"fruit": rgba}, MODEL)["fruit"]
    generate.write_tiers(RELATIVE, master, (4, 2))
    print("ok", master.shape)


if __name__ == "__main__":
    main()
