"""Builds the top-left bar atlas from the ORIGINAL bar art, scaled down: the left end, the middle, the right end with the three
button slots, and the button faces. Nothing is redrawn or added; the pieces are the original pictures resampled (with
premultiplied alpha, so no dark fringes) to their smaller size.

Logical layout of the atlas, 128 x 128 (the size the client sees; every number is a logical pixel):
  left end      (0, 0)    18 x 20
  middle        (24, 0)   22 x 20      stretched to the bar's width by the client
  right end     (0, 26)   74 x 16
  button faces  x = 16 * state, y = 48 + 12 * row, each 14 x 10 (state 0 up, 1 hover, 2 pressed;
                row 0 MU Helper settings, 1 start, 2 stop, 3 Experience Hunter off, 4 on)
Each piece has two extra pixels around it that repeat its edge pixels, so scaling never blends in a neighbour.
The numbers are shared with src/source/UI/NewUI/HUD/TopLeftBarLayout.h; change both together.

Output (relative to the client repository root):
  src/bin/Data/Interface/TopLeftBar/TopLeftBar.OZT         128 x 128
  src/bin/Data/Interface@2x/TopLeftBar/TopLeftBar.png      256 x 256
  src/bin/Data/Interface@4x/TopLeftBar/TopLeftBar.png      512 x 512

The sources are the @2x PNGs of the original bar art in src/bin/Data/Interface@2x (Minimap_positionA/B, MacroUI/*).
Needs Pillow.
"""
import os
import struct
import sys

from PIL import Image

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ATLAS = 128
GUTTER = 2
MASTER = 4  # master scale; the 2x and 1x files are reduced from it

LEFT = (0, 0, 18, 20)
MIDDLE = (24, 0, 22, 20)
RIGHT = (0, 26, 74, 16)
FACE_W, FACE_H = 14, 10
FACE_STRIDE_X, FACE_STRIDE_Y, FACE_ORIGIN_Y = 16, 12, 48
SOURCE_SCALE = 2  # the originals are 2x PNGs


def resize_premultiplied(img, size):
    """LANCZOS resize without colour bleeding from transparent pixels."""
    src = img.convert("RGBA")
    pre = Image.new("RGBA", src.size)
    px, sp0 = pre.load(), src.load()
    for y in range(src.height):
        for x in range(src.width):
            sr, sg, sb, sa = sp0[x, y]
            px[x, y] = (sr * sa // 255, sg * sa // 255, sb * sa // 255, sa)
    small = pre.resize(size, Image.LANCZOS)
    sp = small.load()
    for y in range(small.height):
        for x in range(small.width):
            pr, pg, pb, pa = sp[x, y]
            if pa:
                sp[x, y] = (min(255, pr * 255 // pa), min(255, pg * 255 // pa), min(255, pb * 255 // pa), pa)
            else:
                sp[x, y] = (0, 0, 0, 0)
    return small


def paste_piece(master, piece, x, y, w, h):
    """Paste `piece` (already w*MASTER x h*MASTER) at logical (x, y) and repeat its edge pixels into the gutter."""
    ox, oy = x * MASTER, y * MASTER
    g = GUTTER * MASTER
    master.paste(piece, (ox, oy))
    pw, ph = piece.size
    left = piece.crop((0, 0, 1, ph)).resize((g, ph))
    right = piece.crop((pw - 1, 0, pw, ph)).resize((g, ph))
    master.paste(left, (ox - g, oy))
    master.paste(right, (ox + pw, oy))
    row = master.crop((ox - g, oy, ox + pw + g, oy + ph))
    top = row.crop((0, 0, row.width, 1)).resize((row.width, g))
    bottom = row.crop((0, ph - 1, row.width, ph)).resize((row.width, g))
    master.paste(top, (ox - g, oy - g))
    master.paste(bottom, (ox - g, oy + ph))


def build_master(source):
    master = Image.new("RGBA", (ATLAS * MASTER, ATLAS * MASTER), (0, 0, 0, 0))
    left = Image.open(os.path.join(source, "Minimap_positionA.png"))
    middle = Image.open(os.path.join(source, "Minimap_positionB.png"))
    right = Image.open(os.path.join(source, "MacroUI", "Minimap_positionC_ExpHunt.png"))
    # The original draws the left end 22 x 25 and the middle 26 x 25 (only the top 25 of the texture are art), the right end
    # 91 x 20.
    pieces = (
        (left.crop((0, 0, 22 * SOURCE_SCALE, 25 * SOURCE_SCALE)), LEFT),
        (middle.crop((0, 0, 26 * SOURCE_SCALE, 25 * SOURCE_SCALE)), MIDDLE),
        (right.crop((0, 0, 91 * SOURCE_SCALE, 20 * SOURCE_SCALE)), RIGHT),
    )
    for image, (x, y, w, h) in pieces:
        paste_piece(master, resize_premultiplied(image, (w * MASTER, h * MASTER)), x, y, w, h)
    names = ("Setup", "Start", "Stop", "ExpHuntStart", "ExpHuntStop")
    for row, name in enumerate(names):
        sheet = Image.open(os.path.join(source, "MacroUI", "MacroUI_%s.png" % name))
        for state in range(3):
            face = sheet.crop((0, state * 13 * SOURCE_SCALE, 18 * SOURCE_SCALE, (state + 1) * 13 * SOURCE_SCALE))
            small = resize_premultiplied(face, (FACE_W * MASTER, FACE_H * MASTER))
            paste_piece(master, small, state * FACE_STRIDE_X, FACE_ORIGIN_Y + row * FACE_STRIDE_Y, FACE_W, FACE_H)
    return master


def png_to_ozt(image, destination):
    nx, ny = image.size
    px = image.load()
    header = bytes([0, 0, 2]) + bytes(5) + bytes(2) + bytes(2) + bytes(4)
    header += struct.pack("<h", nx) + struct.pack("<h", ny) + bytes([32, 0])
    body = bytearray()
    for y in range(ny - 1, -1, -1):
        for x in range(nx):
            r, g, b, a = px[x, y]
            body += bytes([b, g, r, a])
    with open(destination, "wb") as f:
        f.write(header)
        f.write(body)
        f.write(bytes(8) + b"TRUEVISION-XFILE.\x00")


def main():
    data = os.path.join(REPO, "src", "bin", "Data")
    source = sys.argv[1] if len(sys.argv) > 1 else os.path.join(data, "Interface@2x")
    master = build_master(source)
    for scale, folder in ((1, "Interface"), (2, "Interface@2x"), (4, "Interface@4x")):
        os.makedirs(os.path.join(data, folder, "TopLeftBar"), exist_ok=True)
        size = ATLAS * scale
        image = master if scale == MASTER else master.resize((size, size), Image.LANCZOS)
        if scale == 1:
            png_to_ozt(image, os.path.join(data, folder, "TopLeftBar", "TopLeftBar.OZT"))
        else:
            image.save(os.path.join(data, folder, "TopLeftBar", "TopLeftBar.png"))
    print("wrote the top-left bar atlas")


if __name__ == "__main__":
    main()
