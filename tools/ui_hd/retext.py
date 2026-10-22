"""Redraws the captioned login-scene buttons with round corners and crisp, font-drawn captions.

The original buttons (Create, Connect, Delete, Menu, Credit) carry a pixel-font caption inside the texture and have
notched corners. Here each state is drawn again from scratch at 4x: a rounded outline, a bevelled ring, a face gradient
per state, and the caption set in the bundled DejaVu Sans Bold. Sizes and state order match the originals exactly
(54 x 30 logical pixels per state, states stacked top to bottom), so the game needs no change.
"""
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

SCALE = 4
SUPERSAMPLE = 2
CELL_WIDTH = 54
CELL_HEIGHT = 30
BUTTON_WIDTH = 44
BUTTON_HEIGHT = 24
CORNER_RADIUS = 5.0
FONT_PATH = os.path.join(os.path.dirname(__file__), "..", "..", "src", "bin", "fonts", "DejaVuSans-Bold.ttf")

OUTLINE_INSET = 0.5
RING_INSET = 1.0
RING_WIDTH = 0.8
GROOVE_WIDTH = 0.5
FACE_INSET = RING_INSET + RING_WIDTH + GROOVE_WIDTH
INNER_FRAME_INSET = 4.0
INNER_FRAME_WIDTH = 0.9
CAPTION_OUTLINE = 1.15
CAPTION_WIDTH = 32.0
CAPTION_HEIGHT = 10.0

BLACK = (8, 8, 8)
GROOVE = (20, 16, 12)
RING_LIGHT = (222, 222, 224)
RING_DARK = (86, 86, 90)
CAPTION_TOP = (255, 250, 214)
CAPTION_BOTTOM = (255, 168, 28)
CAPTION_OUTLINE_COLOUR = (12, 8, 4)

# Face gradients (top, bottom), one per state. Brightness 1.0 = normal.
ORANGE_FACES = [((236, 156, 44), (160, 80, 16)), ((255, 182, 70), (196, 110, 28)), ((176, 100, 26), (112, 56, 10)),
                ((92, 92, 96), (46, 46, 50))]
DARK_FACES = [((52, 46, 40), (16, 14, 12)), ((78, 68, 54), (28, 24, 18)), ((30, 26, 22), (8, 7, 6))]
GRAY_FACES = [((84, 84, 88), (36, 36, 40)), ((118, 118, 124), (58, 58, 64)), ((50, 50, 54), (22, 22, 26))]
CAPTION_FACTORS = {"orange": (1.0, 1.0, 0.72, 0.38), "dark3": (1.0, 1.15, 0.7), "gray": (1.0, 1.0, 0.72)}
PRESSED_STATES = {"orange": (2,), "dark3": (2,), "gray": (2,)}

SPECS = {
    "b_create.OZT": {"label": "Create", "kind": "orange"},
    "b_connect.OZT": {"label": "Connect", "kind": "orange"},
    "b_delete.OZT": {"label": "Delete", "kind": "orange"},
    "server_menu_b_all.OZT": {"label": "Menu", "kind": "gray"},
    "server_credit_b_all.OZT": {"label": "Credit", "kind": "gray"},
    # Three states, and drawn by the game at full cell size (options and message boxes), so no margin.
    "newui_button_close.OZT": {"label": "Close", "kind": "dark3", "full": True},
    "newui_button_cancel.OZT": {"label": "Cancel", "kind": "dark3", "full": True},
    "newui_button_ok.OZT": {"label": "OK", "kind": "dark3", "full": True},
}


def gradient_image(size, top, bottom, mask):
    width, height = size
    rows = np.linspace(0.0, 1.0, height, dtype=np.float32)[:, None, None]
    colours = np.array(top, np.float32) * (1 - rows) + np.array(bottom, np.float32) * rows
    colours = np.broadcast_to(colours, (height, width, 3))
    rgba = np.concatenate([colours, np.asarray(mask, np.float32)[..., None]], axis=2)
    return Image.fromarray(np.clip(rgba, 0, 255).astype(np.uint8), "RGBA")


def rounded_mask(size, inset, radius):
    """Anti-aliased rounded-rectangle mask (0-255) inset by `inset` logical pixels."""
    factor = SCALE * SUPERSAMPLE
    width, height = size[0] * SUPERSAMPLE, size[1] * SUPERSAMPLE
    mask = Image.new("L", (width, height), 0)
    pad = inset * factor
    ImageDraw.Draw(mask).rounded_rectangle([pad, pad, width - 1 - pad, height - 1 - pad],
                                           radius=max(radius * factor - pad, 1), fill=255)
    return np.asarray(mask.resize(size, Image.LANCZOS), np.float32)


def composite(base, layer):
    base.alpha_composite(layer)


def ring_layer(size, pressed):
    outer = rounded_mask(size, RING_INSET, CORNER_RADIUS)
    inner = rounded_mask(size, RING_INSET + RING_WIDTH, CORNER_RADIUS)
    ring = np.clip(outer - inner, 0, 255)
    top, bottom = (RING_DARK, RING_LIGHT) if pressed else (RING_LIGHT, RING_DARK)
    return gradient_image(size, top, bottom, ring)


def button_state(kind, state, face, pressed, button_size=(BUTTON_WIDTH, BUTTON_HEIGHT)):
    size = (button_size[0] * SCALE, button_size[1] * SCALE)
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    composite(image, gradient_image(size, BLACK, BLACK, rounded_mask(size, OUTLINE_INSET, CORNER_RADIUS)))
    composite(image, ring_layer(size, pressed))
    groove = rounded_mask(size, RING_INSET + RING_WIDTH, CORNER_RADIUS) - rounded_mask(size, FACE_INSET - 0.2, CORNER_RADIUS)
    composite(image, gradient_image(size, GROOVE, GROOVE, np.clip(groove, 0, 255)))
    composite(image, gradient_image(size, face[0], face[1], rounded_mask(size, FACE_INSET, CORNER_RADIUS)))
    if kind == "gray":
        frame = rounded_mask(size, INNER_FRAME_INSET, CORNER_RADIUS - 2) - rounded_mask(size, INNER_FRAME_INSET + INNER_FRAME_WIDTH, CORNER_RADIUS - 2)
        composite(image, gradient_image(size, (170, 170, 176), (96, 96, 102), np.clip(frame, 0, 255) * 0.8))
    else:
        shine = rounded_mask(size, FACE_INSET + 0.4, CORNER_RADIUS) - rounded_mask(size, FACE_INSET + 1.3, CORNER_RADIUS)
        top_only = np.zeros_like(shine)
        top_only[: size[1] // 3, :] = 1.0
        composite(image, gradient_image(size, (255, 220, 150), (255, 220, 150), np.clip(shine, 0, 255) * top_only * 0.45))
    return image


def fit_font(label, caption_width=CAPTION_WIDTH, caption_height=CAPTION_HEIGHT):
    size = int(caption_height * SCALE * 1.4)
    while size > 8:
        font = ImageFont.truetype(FONT_PATH, size)
        left, top, right, bottom = font.getbbox(label)
        if right - left <= caption_width * SCALE and (bottom - top) <= caption_height * SCALE:
            return font
        size -= 1
    return ImageFont.truetype(FONT_PATH, 8)


def fit_font_to(label, width, height):
    """Largest DejaVu Sans Bold that fits a width x height box (logical pixels)."""
    size = int(height * SCALE * 1.6)
    while size > 8:
        font = ImageFont.truetype(FONT_PATH, size)
        left, top, right, bottom = font.getbbox(label)
        if right - left <= width * SCALE and (bottom - top) <= height * SCALE:
            return font
        size -= 1
    return ImageFont.truetype(FONT_PATH, 8)


def draw_caption(image, label, factor, caption_width=CAPTION_WIDTH, caption_height=CAPTION_HEIGHT):
    font = fit_font(label, caption_width, caption_height)
    size = image.size
    left, top, right, bottom = font.getbbox(label)
    origin = ((size[0] - (right - left)) // 2 - left, (size[1] - (bottom - top)) // 2 - top - SCALE // 2)
    stroke = max(int(round(CAPTION_OUTLINE * SCALE)), 1)
    outline = Image.new("L", size, 0)
    ImageDraw.Draw(outline).text(origin, label, font=font, fill=255, stroke_width=stroke, stroke_fill=255)
    fill = Image.new("L", size, 0)
    ImageDraw.Draw(fill).text(origin, label, font=font, fill=255)
    top_c = tuple(c * factor for c in CAPTION_TOP)
    bottom_c = tuple(c * factor for c in CAPTION_BOTTOM)
    dark = tuple(c * factor for c in CAPTION_OUTLINE_COLOUR)
    composite(image, gradient_image(size, dark, dark, np.asarray(outline, np.float32)))
    composite(image, gradient_image(size, top_c, bottom_c, np.asarray(fill, np.float32)))


def draw_button(kind, label, full_size=False):
    faces = {"gray": GRAY_FACES, "dark3": DARK_FACES}.get(kind, ORANGE_FACES)
    factors = CAPTION_FACTORS[kind]
    button = (CELL_WIDTH, CELL_HEIGHT) if full_size else (BUTTON_WIDTH, BUTTON_HEIGHT)
    caption = (CAPTION_WIDTH * 1.25, CAPTION_HEIGHT * 1.2) if full_size else (CAPTION_WIDTH, CAPTION_HEIGHT)
    cells = []
    for state, face in enumerate(faces):
        cell = button_state(kind, state, face, state in PRESSED_STATES[kind], button)
        draw_caption(cell, label, factors[state], *caption)
        full = Image.new("RGBA", (CELL_WIDTH * SCALE, CELL_HEIGHT * SCALE), (0, 0, 0, 0))
        full.alpha_composite(cell, ((CELL_WIDTH - button[0]) * SCALE // 2, (CELL_HEIGHT - button[1]) * SCALE // 2))
        cells.append(np.asarray(full))
    return np.concatenate(cells, axis=0)


def apply(name, master):
    """Returns the redrawn 4x button for a captioned texture; every other texture passes through unchanged."""
    spec = SPECS.get(name)
    if spec is None:
        return master
    return draw_button(spec["kind"], spec["label"], spec.get("full", False))
