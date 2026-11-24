"""Builds the Experience Hunter bar atlas from the ORIGINAL bar art (Minimap_positionA / B), scaled down.

Nothing is redrawn or added: the left end, the stretched middle and the same end mirrored are the original pictures
resampled (with premultiplied alpha, so no dark fringes) to the smaller size.

Logical layout of the atlas, 64 x 64 (every number is a logical pixel):
  left end   (2, 2)    16 x 18
  right end  (22, 2)   16 x 18     the left end mirrored
  middle     (42, 2)   16 x 18     stretched to the bar's width by the client
Each piece has two extra pixels around it that repeat its edge pixels, so scaling never blends in a neighbour.
The numbers are shared with src/source/UI/NewUI/HUD/ExpHuntBarLayout.h; change both together.

Output (relative to the client repository root):
  src/bin/Data/Interface/ExpHuntBar/ExpHuntBar.OZT         64 x 64
  src/bin/Data/Interface@2x/ExpHuntBar/ExpHuntBar.png      128 x 128
  src/bin/Data/Interface@4x/ExpHuntBar/ExpHuntBar.png      256 x 256

The sources are the @2x PNGs of the original art in src/bin/Data/Interface@2x. Needs Pillow.
"""
import os
import sys

from PIL import Image, ImageOps

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(REPO, "tools", "top_left_bar"))
from make_top_left_bar import png_to_ozt, resize_premultiplied  # noqa: E402

ATLAS = 64
GUTTER = 2
MASTER = 4  # master scale; the 2x and 1x files are reduced from it
SOURCE_SCALE = 2  # the originals are 2x PNGs

W, H = 16, 18
LEFT = (2, 2)
RIGHT = (22, 2)
MIDDLE = (42, 2)


def paste_piece(master, piece, x, y):
    """Paste `piece` (W*MASTER x H*MASTER) at logical (x, y) and repeat its edge pixels into the gutter."""
    ox, oy = x * MASTER, y * MASTER
    g = GUTTER * MASTER
    master.paste(piece, (ox, oy))
    pw, ph = piece.size
    master.paste(piece.crop((0, 0, 1, ph)).resize((g, ph)), (ox - g, oy))
    master.paste(piece.crop((pw - 1, 0, pw, ph)).resize((g, ph)), (ox + pw, oy))
    row = master.crop((ox - g, oy, ox + pw + g, oy + ph))
    master.paste(row.crop((0, 0, row.width, 1)).resize((row.width, g)), (ox - g, oy - g))
    master.paste(row.crop((0, ph - 1, row.width, ph)).resize((row.width, g)), (ox - g, oy + ph))


def build_master(source):
    master = Image.new("RGBA", (ATLAS * MASTER, ATLAS * MASTER), (0, 0, 0, 0))
    cap_source = Image.open(os.path.join(source, "Minimap_positionA.png")).crop(
        (0, 0, 22 * SOURCE_SCALE, 25 * SOURCE_SCALE)
    )
    middle_source = Image.open(os.path.join(source, "Minimap_positionB.png")).crop(
        (0, 0, 26 * SOURCE_SCALE, 25 * SOURCE_SCALE)
    )
    cap = resize_premultiplied(cap_source, (W * MASTER, H * MASTER))
    middle = resize_premultiplied(middle_source, (W * MASTER, H * MASTER))
    paste_piece(master, cap, *LEFT)
    paste_piece(master, ImageOps.mirror(cap), *RIGHT)
    paste_piece(master, middle, *MIDDLE)
    return master


def main():
    data = os.path.join(REPO, "src", "bin", "Data")
    source = sys.argv[1] if len(sys.argv) > 1 else os.path.join(data, "Interface@2x")
    master = build_master(source)
    for scale, folder in ((1, "Interface"), (2, "Interface@2x"), (4, "Interface@4x")):
        os.makedirs(os.path.join(data, folder, "ExpHuntBar"), exist_ok=True)
        size = ATLAS * scale
        image = master if scale == MASTER else master.resize((size, size), Image.LANCZOS)
        if scale == 1:
            png_to_ozt(image, os.path.join(data, folder, "ExpHuntBar", "ExpHuntBar.OZT"))
        else:
            image.save(os.path.join(data, folder, "ExpHuntBar", "ExpHuntBar.png"))
    print("wrote the Experience Hunter bar atlas")


if __name__ == "__main__":
    main()
