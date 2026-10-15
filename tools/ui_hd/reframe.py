"""Gives square-cornered, notched button textures a round outline while keeping their original face and icon.

For each state cell the old frame is cleared and a rounded, bevelled ring is drawn again at 4x; the original face
(the stone and its icon) is fitted inside it. Used for the bottom-right menu buttons and the skill boxes, whose frames
are baked into JPEG textures. Everything stays opaque, as the originals were.
"""
import numpy as np
from PIL import Image

import retext
from retext import SCALE, FACE_INSET, GROOVE, RING_INSET, RING_WIDTH, composite, gradient_image, rounded_mask

CORNER_RADIUS = 5.0
GROOVE_BLEND = 0.9

# rect: the button inside one cell; crop: the part of the original face reused inside the new frame (cell pixels).
SPECS = {
    "partCharge1/newui_menu_Bt01.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36)},
    "partCharge1/newui_menu_Bt02.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36)},
    "partCharge1/newui_menu_Bt03.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36)},
    "partCharge1/newui_menu_Bt04.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36)},
    "partCharge1/newui_menu_Bt05.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36)},
    "newui_skillbox.OZJ": {"cells": 1, "cell_height": 38, "rect": (0, 0, 32, 38), "crop": (4, 4, 28, 34), "transparent_corners": True},
    "newui_skillbox2.OZJ": {"cells": 1, "cell_height": 38, "rect": (0, 0, 32, 38), "crop": (4, 4, 28, 34), "transparent_corners": True},
}


# Hotkey slots baked into the bar strips: (x0, y0, x1, y1) of each frame, in texture pixels, with its caption.
SLOT_Y0, SLOT_Y1 = 3, 40
SLOT_STRIPS = {
    "newui_menu01.OZJ": [((0, SLOT_Y0, 38, SLOT_Y1), "Q"), ((38, SLOT_Y0, 76, SLOT_Y1), "W"),
                         ((76, SLOT_Y0, 114, SLOT_Y1), "E"), ((114, SLOT_Y0, 152, SLOT_Y1), "R"),
                         ((222, SLOT_Y0, 253, SLOT_Y1), None)],
    "newui_menu02.OZJ": [((0, SLOT_Y0, 31, SLOT_Y1), None), ((31, SLOT_Y0, 63, SLOT_Y1), None),
                         ((63, SLOT_Y0, 95, SLOT_Y1), None), ((95, SLOT_Y0, 127, SLOT_Y1), None)],
}
WIDEST_CAPTION = "W"
SLOT_CROP_INSET = 4
CAPTION_BOX = (3.0, 4.0, 12.0, 14.0)
CAPTION_COLOUR_TOP = (255, 242, 150)
CAPTION_COLOUR_BOTTOM = (255, 196, 30)


def scaled(values):
    return [int(round(v * SCALE)) for v in values]


def frame_layers(size, face):
    """Black outline, bevel ring, groove, then `face` (RGBA of `size`) inside the rounded face area."""
    radius = CORNER_RADIUS
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    composite(image, gradient_image(size, (6, 6, 6), (6, 6, 6), rounded_mask_r(size, 0.0, radius)))
    outer, inner = rounded_mask_r(size, RING_INSET, radius), rounded_mask_r(size, RING_INSET + RING_WIDTH, radius)
    composite(image, gradient_image(size, retext.RING_LIGHT, retext.RING_DARK, np.clip(outer - inner, 0, 255)))
    groove = inner - rounded_mask_r(size, FACE_INSET - 0.2, radius)
    composite(image, gradient_image(size, GROOVE, GROOVE, np.clip(groove, 0, 255)))
    face_mask = rounded_mask_r(size, FACE_INSET, radius) / 255.0
    layer = np.asarray(face, np.float32).copy()
    layer[..., 3] = face_mask * 255.0
    composite(image, Image.fromarray(np.clip(layer, 0, 255).astype(np.uint8), "RGBA"))
    return image


def rounded_mask_r(size, inset, radius):
    saved = retext.CORNER_RADIUS
    retext.CORNER_RADIUS = radius
    try:
        return rounded_mask(size, inset, radius)
    finally:
        retext.CORNER_RADIUS = saved


def reframe_cell(cell, spec):
    x0, y0, x1, y1 = scaled(spec["rect"])
    cx0, cy0, cx1, cy1 = scaled(spec["crop"])
    size = (x1 - x0, y1 - y0)
    face_area = (round(FACE_INSET * SCALE), round(FACE_INSET * SCALE),
                 size[0] - round(FACE_INSET * SCALE), size[1] - round(FACE_INSET * SCALE))
    source = Image.fromarray(cell[cy0:cy1, cx0:cx1]).resize(
        (face_area[2] - face_area[0], face_area[3] - face_area[1]), Image.LANCZOS)
    face = Image.new("RGBA", size, (0, 0, 0, 255))
    face.paste(source, face_area[:2])
    out = cell.copy()
    out[y0:y1, x0:x1] = (0, 0, 0, 255)
    framed = frame_layers(size, face)
    base = Image.fromarray(out[y0:y1, x0:x1], "RGBA")
    base.alpha_composite(framed)
    out[y0:y1, x0:x1] = np.asarray(base)
    out[..., 3] = 255
    if spec.get("transparent_corners"):
        coverage = rounded_mask_r(size, 0.0, CORNER_RADIUS) / 255.0
        out[y0:y1, x0:x1, 3] = np.clip(np.rint(coverage * 255.0), 0, 255).astype(np.uint8)
    return out


# Outer corners of the whole bottom bar: texture -> corners to round ("tl", "tr").
OUTER_CORNER_RADIUS = 7.0
OUTER_STROKE = 0.8
OUTER_STROKE_COLOUR = (206, 176, 124)
OUTER_CORNERS = {
    "newui_menu01.OZJ": ("tl",),
    "partCharge1/newui_menu03.OZJ": ("tr",),
}


def round_outer_corners(master, corners):
    """Cuts the listed top corners of a bar piece to a quarter circle (transparent outside) and strokes the curve."""
    out = master.copy()
    height, width = out.shape[:2]
    radius = int(round(OUTER_CORNER_RADIUS * SCALE))
    inset = OUTER_STROKE * SCALE
    for corner in corners:
        x0 = 0 if corner == "tl" else width - radius
        block = out[0:radius, x0:x0 + radius].copy()
        full = (radius * 2, radius * 2)
        outer = rounded_mask_r(full, 0.0, OUTER_CORNER_RADIUS * 1.0)
        inner = rounded_mask_r(full, OUTER_STROKE, OUTER_CORNER_RADIUS * 1.0)
        outer = outer[:radius, :radius] if corner == "tl" else outer[:radius, radius:]
        inner = inner[:radius, :radius] if corner == "tl" else inner[:radius, radius:]
        keep = outer / 255.0
        stroke = np.clip(outer - inner, 0, 255) / 255.0
        colour = np.array(OUTER_STROKE_COLOUR, np.float32)
        rgb = block[..., :3].astype(np.float32) * (1 - stroke[..., None]) + colour * stroke[..., None]
        block[..., :3] = np.clip(np.rint(rgb * keep[..., None]), 0, 255).astype(np.uint8)
        block[..., 3] = np.clip(np.rint(keep * 255.0), 0, 255).astype(np.uint8)
        out[0:radius, x0:x0 + radius] = block
    return out


def slot_spec(rect):
    x0, y0, x1, y1 = rect
    inset = SLOT_CROP_INSET
    return {"rect": rect, "crop": (x0 + inset, y0 + inset, x1 - inset, y1 - inset)}


def apply_slots(name, master):
    out = master.copy()
    for rect, caption in SLOT_STRIPS[name]:
        spec = slot_spec(rect)
        x0, y0, x1, y1 = scaled(rect)
        cell = out.copy()
        if caption:
            cell = erase_caption_area(cell, rect)
        framed = reframe_cell(cell, spec)
        out[y0:y1, x0:x1] = framed[y0:y1, x0:x1]
        if caption:
            draw_slot_caption(out, rect, caption)
    return out


def erase_caption_area(cell, rect):
    """Paints the old pixel-font letter of a slot with the slot face (filled from the pixels just right and below)."""
    x0, y0, _, _ = rect
    bx0, by0, bx1, by1 = scaled((x0 + CAPTION_BOX[0], y0 + CAPTION_BOX[1] - 3, x0 + CAPTION_BOX[2], y0 + CAPTION_BOX[3] - 3))
    out = cell.copy()
    width, height = bx1 - bx0, by1 - by0
    right = out[by0:by1, bx1:bx1 + SCALE * 2, :3].astype(np.float32).mean(axis=1)
    fill = np.repeat(right[:, None, :], width, axis=1)
    out[by0:by1, bx0:bx1, :3] = np.clip(np.rint(fill), 0, 255).astype(np.uint8)
    return out


def draw_slot_caption(out, rect, caption):
    from PIL import ImageDraw
    x0, y0, _, _ = rect
    bx0, by0, bx1, by1 = scaled((x0 + CAPTION_BOX[0], y0 + CAPTION_BOX[1] - 1.5, x0 + CAPTION_BOX[2], y0 + CAPTION_BOX[3] - 1.5))
    size = (bx1 - bx0, by1 - by0)
    font = retext.fit_font_to(WIDEST_CAPTION, size[0] / SCALE - 1.0, size[1] / SCALE - 1.0)
    left, top, right, bottom = font.getbbox(caption)
    origin = ((size[0] - (right - left)) // 2 - left, (size[1] - (bottom - top)) // 2 - top)
    stroke = max(int(round(retext.CAPTION_OUTLINE * SCALE * 0.8)), 1)
    outline = Image.new("L", size, 0)
    ImageDraw.Draw(outline).text(origin, caption, font=font, fill=255, stroke_width=stroke, stroke_fill=255)
    fill = Image.new("L", size, 0)
    ImageDraw.Draw(fill).text(origin, caption, font=font, fill=255)
    region = Image.fromarray(out[by0:by1, bx0:bx1].copy(), "RGBA")
    composite(region, gradient_image(size, (8, 6, 2), (8, 6, 2), np.asarray(outline, np.float32)))
    composite(region, gradient_image(size, CAPTION_COLOUR_TOP, CAPTION_COLOUR_BOTTOM, np.asarray(fill, np.float32)))
    out[by0:by1, bx0:bx1] = np.asarray(region)


def apply(name, master):
    if name in OUTER_CORNERS:
        return round_outer_corners(apply_inner(name, master), OUTER_CORNERS[name])
    return apply_inner(name, master)


def apply_inner(name, master):
    if name in SLOT_STRIPS:
        return apply_slots(name, master)
    spec = SPECS.get(name)
    if spec is None:
        return master
    height = spec["cell_height"] * SCALE
    cells = [reframe_cell(master[i * height:(i + 1) * height], spec) for i in range(spec["cells"])]
    rest = master[spec["cells"] * height:]
    return np.concatenate(cells + ([rest] if len(rest) else []), axis=0)
