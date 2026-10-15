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
AUTO_RECT = "auto"  # rect taken from the visible part of each cell
GROOVE_BLEND = 0.9

# rect: the button inside one cell; crop: the part of the original face reused inside the new frame (cell pixels).
SPECS = {
    "partCharge1/newui_menu_Bt01.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36), "transparent_corners": True},
    "partCharge1/newui_menu_Bt02.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36), "transparent_corners": True},
    "partCharge1/newui_menu_Bt03.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36), "transparent_corners": True},
    "partCharge1/newui_menu_Bt04.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36), "transparent_corners": True},
    "partCharge1/newui_menu_Bt05.OZJ": {"cells": 4, "cell_height": 41, "rect": (0, 3, 30, 41), "crop": (5, 7, 25, 36), "transparent_corners": True},
    "newui_Bt_Chat_guild.OZJ": {"cells": 2, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_Bt_Chat_normal.OZJ": {"cells": 2, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_Bt_Chat_party.OZJ": {"cells": 2, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_Bt_Chat_system.OZJ": {"cells": 2, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_btn_alpha.OZJ": {"cells": 2, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_btn_size.OZJ": {"cells": 2, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_chat_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_frame_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_gens_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_guild_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_normal_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_party_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_system_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_chat_whisper_on.OZJ": {"cells": 1, "cell_height": 26, "rect": (0, 0, 27, 26), "crop": (4, 4, 23, 22), "transparent_corners": True},
    "newui_bt_Off_chat.OZJ": {"cells": 1, "cell_height": 23, "rect": (0, 0, 22, 23), "crop": (4, 4, 18, 19), "transparent_corners": True},
    "newui_exit_00.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_repair_00.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_expansion_btn.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_Bt_openshop.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_Bt_closeshop.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_chainfo_btn_quest.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_chainfo_btn_pet.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_chainfo_btn_master.OZT": {"cells": 2, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin"},
    "newui_btn_empty.OZT": {"cells": 3, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin", "shrink": 1.5},
    "newui_btn_empty_big.OZT": {"cells": 3, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin", "shrink": 1.5},
    "newui_btn_empty_small.OZT": {"cells": 3, "cell_height": 29, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin", "shrink": 1.5},
    "newui_btn_empty_very_small.OZT": {"cells": 3, "cell_height": 23, "rect": AUTO_RECT, "crop": None, "transparent_corners": True, "profile": "thin", "shrink": 1.5},
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


# Frame profiles: (ring inset, ring width, groove width) in logical pixels. "thin" keeps the round corners with a light frame.
PROFILES = {
    "normal": (RING_INSET, RING_WIDTH, retext.GROOVE_WIDTH),
    "thin": (0.6, 0.8, 0.4),
}


def face_inset(profile):
    inset, ring, groove = PROFILES[profile]
    return inset + ring + groove


def frame_layers(size, face, profile="normal"):
    """Black outline, bevel ring, groove, then `face` (RGBA of `size`) inside the rounded face area."""
    ring_inset, ring_width, groove_width = PROFILES[profile]
    FACE_INSET = face_inset(profile)
    radius = CORNER_RADIUS
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    composite(image, gradient_image(size, (6, 6, 6), (6, 6, 6), rounded_mask_r(size, 0.0, radius)))
    outer, inner = rounded_mask_r(size, ring_inset, radius), rounded_mask_r(size, ring_inset + ring_width, radius)
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


AUTO_CROP_MARGIN = 3


def opaque_box(cell):
    """Bounding box (x0, y0, x1, y1) in texels of the visible part of a cell; the original art keeps a transparent margin."""
    ys, xs = np.nonzero(cell[..., 3] > 128)
    return int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1


def cell_boxes(cell, spec):
    if spec["rect"] != AUTO_RECT:
        return scaled(spec["rect"]), scaled(spec["crop"])
    x0, y0, x1, y1 = opaque_box(cell)
    shrink = int(round(spec.get("shrink", 0.0) * SCALE))
    x0, y0, x1, y1 = x0 + shrink, y0 + shrink, x1 - shrink, y1 - shrink
    margin = AUTO_CROP_MARGIN * SCALE
    return (x0, y0, x1, y1), (x0 + margin, y0 + margin, x1 - margin, y1 - margin)


def reframe_cell(cell, spec):
    (x0, y0, x1, y1), (cx0, cy0, cx1, cy1) = cell_boxes(cell, spec)
    size = (x1 - x0, y1 - y0)
    inset = face_inset(spec.get("profile", "normal"))
    face_area = (round(inset * SCALE), round(inset * SCALE),
                 size[0] - round(inset * SCALE), size[1] - round(inset * SCALE))
    source = Image.fromarray(cell[cy0:cy1, cx0:cx1]).resize(
        (face_area[2] - face_area[0], face_area[3] - face_area[1]), Image.LANCZOS)
    face = Image.new("RGBA", size, (0, 0, 0, 255))
    face.paste(source, face_area[:2])
    out = cell.copy()
    out[y0:y1, x0:x1] = (0, 0, 0, 255)
    framed = frame_layers(size, face, spec.get("profile", "normal"))
    base = Image.fromarray(out[y0:y1, x0:x1], "RGBA")
    base.alpha_composite(framed)
    out[y0:y1, x0:x1] = np.asarray(base)
    out[..., 3] = 255
    if spec.get("transparent_corners"):
        coverage = rounded_mask_r(size, 0.0, CORNER_RADIUS) / 255.0
        out[..., 3] = 0
        out[y0:y1, x0:x1, 3] = np.clip(np.rint(coverage * 255.0), 0, 255).astype(np.uint8)
    return out


# Outer corners of the whole bottom bar: texture -> corners to round ("tl", "tr").
OUTER_CORNER_RADIUS = 9.0
OUTER_STROKE = 1.0
OUTER_STROKE_COLOUR = (150, 152, 160)
OUTER_CORNERS = {
    "newui_menu01.OZJ": ("tl",),
    "partCharge1/newui_menu03.OZJ": ("tr",),
    "newui_item_back03.OZT": ("bl", "br"),
}


def round_outer_corners(master, corners):
    """Cuts the listed corners ("tl", "tr", "bl", "br") to a quarter circle (transparent outside) and strokes the curve."""
    out = master.copy()
    for corner in corners:
        flipped = corner[0] == "b"
        work = out[::-1].copy() if flipped else out
        work = round_top_corner(work, "tl" if corner[1] == "l" else "tr")
        out = work[::-1].copy() if flipped else work
    return out


def round_top_corner(master, corner):
    out = master.copy()
    height, width = out.shape[:2]
    radius = int(round(OUTER_CORNER_RADIUS * SCALE))
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
    block[..., 3] = np.clip(np.rint(block[..., 3].astype(np.float32) * keep), 0, 255).astype(np.uint8)
    out[0:radius, x0:x0 + radius] = block
    return out


# The thin gold stripe along the top of the bar pieces is recoloured to steel, like the slot rings.
TOP_LINE_TEXTURES = {"newui_menu01.OZJ", "newui_menu02.OZJ", "partCharge1/newui_menu03.OZJ", "newui_menu02-03.OZJ"}
TOP_LINE_ROWS = 4
TOP_LINE_MIN_WARMTH = 25
STEEL_TINT = (0.92, 0.94, 1.0)


def recolour_top_line(master):
    out = master.copy()
    band = out[:TOP_LINE_ROWS * SCALE, :, :3].astype(np.float32)
    warm = (band[..., 0] - band[..., 2] > TOP_LINE_MIN_WARMTH) & (band[..., 0] >= band[..., 1])
    luma = band[..., 0] * 0.3 + band[..., 1] * 0.59 + band[..., 2] * 0.11
    steel = np.stack([luma * STEEL_TINT[0], luma * STEEL_TINT[1], luma * STEEL_TINT[2]], axis=2)
    band[warm] = steel[warm]
    out[:TOP_LINE_ROWS * SCALE, :, :3] = np.clip(np.rint(band), 0, 255).astype(np.uint8)
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
    if name in TOP_LINE_TEXTURES:
        master = recolour_top_line(master)
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
