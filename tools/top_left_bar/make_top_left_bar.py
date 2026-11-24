"""Generates the top-left HUD bar art: the compact frame (map name above, coordinates and three buttons below) and the
button faces, all in one atlas so the bar is a single texture.

Logical layout of the atlas, 128 x 128 (the size the client sees; every coordinate below is a logical pixel):
  frame          (0, 0)  104 x 34
  button faces   cell (col, row) at x = 16 * col, y = 40 + 16 * row, each 14 x 14, with a 2 px empty gutter around it
                 col 0 up, col 1 hover, col 2 pressed
                 row 0 MU Helper settings (wrench), 1 MU Helper start (play), 2 MU Helper stop (pause, lit),
                 3 Experience Hunter off (XP), 4 Experience Hunter on (XP, lit)
The numbers are shared with src/source/UI/NewUI/HUD/TopLeftBarLayout.h; change both together.

The art is drawn at 8x and reduced; every straight edge sits on a whole logical pixel, so the 1x texture is as crisp as
the 2x and 4x ones.

Output (relative to the client repository root):
  src/bin/Data/Interface/TopLeftBar/TopLeftBar.OZT           128 x 128
  src/bin/Data/Interface@2x/TopLeftBar/TopLeftBar.png        256 x 256
  src/bin/Data/Interface@4x/TopLeftBar/TopLeftBar.png        512 x 512

Needs Pillow:  python tools/top_left_bar/make_top_left_bar.py
"""
import os
import struct

from PIL import Image, ImageDraw, ImageFont

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SS = 8
ATLAS = 128
FRAME_W, FRAME_H = 104, 34
BTN = 14
CELL = 16
BTN_ORIGIN_Y = 40
NAME_ROW_BOTTOM = 16  # y of the etched divider
INSET = (4, 18, 50, 32)  # coordinate readout: x0, y0, x1, y1
BUTTON_X = (54, 70, 86)  # only used by the preview; the client positions the buttons from the layout header

INK = (6, 6, 8, 255)
SILVER_HI = (190, 190, 198, 255)
SILVER = (140, 140, 148, 255)
SILVER_LO = (78, 78, 86, 255)
STEEL_TOP = (46, 46, 52, 255)
STEEL_BOT = (22, 22, 26, 255)
AMBER = (255, 168, 54, 255)
AMBER_HI = (255, 214, 120, 255)
AMBER_LO = (150, 78, 14, 255)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(len(a)))


class Canvas:
    """RGBA canvas addressed in logical pixels; everything is drawn at SS times the size."""

    def __init__(self, w, h):
        self.img = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def rect(self, x0, y0, x1, y1, fill):
        self.d.rectangle((x0 * SS, y0 * SS, x1 * SS - 1, y1 * SS - 1), fill=fill)

    def vgrad(self, x0, y0, x1, y1, top, bottom):
        rows = (y1 - y0) * SS
        for i in range(rows):
            c = lerp(top, bottom, i / max(1, rows - 1))
            self.d.rectangle((x0 * SS, y0 * SS + i, x1 * SS - 1, y0 * SS + i), fill=c)

    def poly(self, pts, fill):
        self.d.polygon([(x * SS, y * SS) for x, y in pts], fill=fill)

    def line(self, pts, fill, width):
        self.d.line([(x * SS, y * SS) for x, y in pts], fill=fill, width=int(width * SS), joint="curve")

    def ellipse(self, x0, y0, x1, y1, fill):
        self.d.ellipse((x0 * SS, y0 * SS, x1 * SS - 1, y1 * SS - 1), fill=fill)

    def arc(self, box, start, end, fill, width):
        x0, y0, x1, y1 = box
        self.d.arc((x0 * SS, y0 * SS, x1 * SS, y1 * SS), start, end, fill=fill, width=int(width * SS))


def bevel_box(c, x0, y0, x1, y1, top, bottom, hi, lo, outline=INK, inverted=False):
    """Outlined box with a one pixel bevel: light on the top/left, dark on the bottom/right (swapped when inverted)."""
    light, dark = (lo, hi) if inverted else (hi, lo)
    c.rect(x0, y0, x1, y1, outline)
    c.vgrad(x0 + 1, y0 + 1, x1 - 1, y1 - 1, top, bottom)
    c.rect(x0 + 1, y0 + 1, x1 - 1, y0 + 2, light)  # top edge
    c.rect(x0 + 1, y0 + 1, x0 + 2, y1 - 1, light)  # left edge
    c.rect(x0 + 1, y1 - 2, x1 - 1, y1 - 1, dark)  # bottom edge
    c.rect(x1 - 2, y0 + 1, x1 - 1, y1 - 1, dark)  # right edge


def curl(c, x, y, flip_x, flip_y, colour=SILVER):
    """A small scroll ornament in the corner at (x, y), curling inwards."""
    sx = -1 if flip_x else 1
    sy = -1 if flip_y else 1

    def pt(px, py):
        return (x + sx * px, y + sy * py)

    c.line([pt(1.0, 6.0), pt(1.0, 3.0), pt(2.0, 1.5), pt(4.0, 1.0), pt(6.0, 1.0)], colour, 0.9)
    c.line([pt(2.5, 4.5), pt(2.6, 3.4), pt(3.6, 2.8)], colour, 0.7)
    cx, cy = pt(3.8, 3.8)
    c.ellipse(cx - 0.55, cy - 0.55, cx + 0.55, cy + 0.55, SILVER_HI)


def draw_frame():
    c = Canvas(FRAME_W, FRAME_H)
    bevel_box(c, 0, 0, FRAME_W, FRAME_H, STEEL_TOP, STEEL_BOT, SILVER, SILVER_LO)
    # inner soft shadow under the top bevel and a second hairline for the "plate" feel
    c.rect(3, 3, FRAME_W - 3, 4, (12, 12, 15, 255))
    c.rect(3, FRAME_H - 4, FRAME_W - 3, FRAME_H - 3, (34, 34, 40, 255))
    # corner ornaments
    curl(c, 3.0, 3.0, False, False)
    curl(c, FRAME_W - 3.0, 3.0, True, False)
    # etched divider between the name row and the readout row, with a small centre diamond
    mid = FRAME_W / 2
    c.rect(9, NAME_ROW_BOTTOM, FRAME_W - 9, NAME_ROW_BOTTOM + 1, INK)
    c.rect(9, NAME_ROW_BOTTOM + 1, FRAME_W - 9, NAME_ROW_BOTTOM + 2, (70, 70, 78, 255))
    c.poly([(mid - 2.5, NAME_ROW_BOTTOM + 0.5), (mid, NAME_ROW_BOTTOM - 1.5), (mid + 2.5, NAME_ROW_BOTTOM + 0.5),
            (mid, NAME_ROW_BOTTOM + 2.5)], AMBER)
    c.poly([(mid - 1.2, NAME_ROW_BOTTOM + 0.5), (mid, NAME_ROW_BOTTOM - 0.5), (mid + 1.2, NAME_ROW_BOTTOM + 0.5),
            (mid, NAME_ROW_BOTTOM + 1.5)], AMBER_HI)
    # recessed coordinate readout
    x0, y0, x1, y1 = INSET
    bevel_box(c, x0, y0, x1, y1, (8, 8, 10, 255), (14, 14, 17, 255), (4, 4, 6, 255), (62, 62, 70, 255), inverted=False)
    return c.img


def glyph_wrench(c, ox, oy, colour, shade):
    """A wrench along the diagonal, drawn on its own layer and rotated onto the plate."""
    layer = Canvas(14, 14)
    layer.rect(5.6, 4, 8.4, 12, colour)  # handle
    layer.ellipse(3.6, 0.8, 10.4, 7.6, colour)  # head
    layer.rect(6.0, 0.0, 8.0, 3.8, (0, 0, 0, 0))  # jaw opening
    layer.rect(6.6, 4.4, 7.4, 11.4, shade)
    layer.ellipse(5.8, 11.0, 8.2, 13.4, colour)
    rot = layer.img.rotate(-45, resample=Image.BICUBIC, center=(7 * SS, 7 * SS))
    c.img.alpha_composite(rot, (int(ox * SS), int(oy * SS)))


def glyph_play(c, ox, oy, fill, hi):
    pts = [(ox + 4.5, oy + 3.0), (ox + 10.5, oy + 7.0), (ox + 4.5, oy + 11.0)]
    c.poly(pts, fill)
    c.poly([(ox + 4.5, oy + 3.0), (ox + 10.5, oy + 7.0), (ox + 9.0, oy + 7.0), (ox + 4.5, oy + 5.0)], hi)


def glyph_pause(c, ox, oy, fill, hi):
    for bx in (4.0, 8.0):
        c.rect(ox + bx, oy + 3.0, ox + bx + 2.0, oy + 11.0, fill)
        c.rect(ox + bx, oy + 3.0, ox + bx + 0.8, oy + 11.0, hi)


def glyph_xp(c, ox, oy, fill, shadow):
    font = ImageFont.truetype("C:/Windows/Fonts/arialbd.ttf", int(7.6 * SS))
    for dx, dy, col in ((0.0, 0.7, shadow), (0.0, 0.0, fill)):
        c.d.text(((ox + 7.0 + dx) * SS, (oy + 7.0 + dy) * SS), "XP", font=font, fill=col, anchor="mm")


PLATE = {
    "up": ((58, 58, 66, 255), (28, 28, 33, 255)),
    "over": ((86, 80, 78, 255), (44, 38, 36, 255)),
    "down": ((18, 18, 22, 255), (30, 30, 36, 255)),
}
PLATE_LIT = {
    "up": ((96, 60, 20, 255), (46, 26, 8, 255)),
    "over": ((128, 82, 28, 255), (64, 36, 10, 255)),
    "down": ((34, 20, 6, 255), (58, 34, 10, 255)),
}


def draw_button(c, ox, oy, row, state):
    lit = row in (2, 4)
    top, bottom = (PLATE_LIT if lit else PLATE)[state]
    edge_hi = AMBER_HI if lit else SILVER_HI
    edge_lo = AMBER_LO if lit else SILVER_LO
    if state == "over":
        edge_hi = (255, 236, 190, 255) if lit else (225, 225, 232, 255)
    bevel_box(c, ox, oy, ox + BTN, oy + BTN, top, bottom, edge_hi, edge_lo, inverted=(state == "down"))
    nudge = 1 if state == "down" else 0
    gx, gy = ox + 0 + nudge * 0.5, oy + nudge * 0.5
    gold = (255, 200, 90, 255)
    silver = (206, 206, 214, 255)
    if row == 0:
        glyph_wrench(c, gx, gy, silver, (120, 120, 130, 255))
    elif row == 1:
        glyph_play(c, gx, gy, AMBER, AMBER_HI)
    elif row == 2:
        glyph_pause(c, gx, gy, AMBER_HI, (255, 244, 214, 255))
    elif row == 3:
        glyph_xp(c, gx, gy, (176, 176, 186, 255), INK)
    else:
        glyph_xp(c, gx, gy, gold, (60, 28, 4, 255))


def build_master():
    atlas = Canvas(ATLAS, ATLAS)
    atlas.img.alpha_composite(draw_frame(), (0, 0))
    for row in range(5):
        for col, state in enumerate(("up", "over", "down")):
            draw_button(atlas, CELL * col, BTN_ORIGIN_Y + CELL * row, row, state)
    return atlas.img


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
    master = build_master()
    for scale, folder in ((1, "Interface"), (2, "Interface@2x"), (4, "Interface@4x")):
        os.makedirs(os.path.join(data, folder, "TopLeftBar"), exist_ok=True)
        image = master.resize((ATLAS * scale, ATLAS * scale), Image.LANCZOS)
        if scale == 1:
            png_to_ozt(image, os.path.join(data, folder, "TopLeftBar", "TopLeftBar.OZT"))
        else:
            image.save(os.path.join(data, folder, "TopLeftBar", "TopLeftBar.png"))
    print("wrote the top-left bar atlas")


if __name__ == "__main__":
    main()
