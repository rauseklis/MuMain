"""Redraws the Character window's stat rows (label plate + value box) and the orange "+" buttons at 4x.

The originals are tiny pixel art: a stone plate with a black value box, and a 16 x 15 orange button with a plus. An
enlargement keeps the pixel noise and washes the colours out, so both are drawn again from measured colours:
  newui_cha_textbox02    170 x 21   stone label plate (left) and a black value box (right) in a thin steel frame
  newui_chainfo_btn_level 16 x 30   two stacked 16 x 15 buttons: normal (orange) and disabled (grey-brown)
"""
import numpy as np
from PIL import Image, ImageDraw

from upscale import gaussian_blur

SCALE = 4
SUPERSAMPLE = 3
TEXTBOX = "newui_cha_textbox02.ozt"
PLUS_BUTTON = "newui_chainfo_btn_level.ozt"
SPECS = {TEXTBOX, PLUS_BUTTON}

BOX_CORNER = 3.0
FRAME_WIDTH = 0.9
DIVIDER_X = 73.0
VALUE_INSET = 1.4
FRAME_LIGHT = (196, 198, 204)
FRAME_DARK = (84, 86, 92)
PLATE_TOP = (88, 88, 90)
PLATE_BOTTOM = (46, 46, 48)
VALUE_FILL = (0, 0, 0)
GRAIN_STRENGTH = 5.0
GRAIN_SEED = 31

BUTTON_CELL = (16, 15)
BUTTON_CORNER = 3.2
BUTTON_FACES = [((255, 196, 84), (196, 98, 20)), ((150, 126, 96), (96, 74, 52))]
BUTTON_EDGE = [(74, 34, 8), (52, 40, 28)]
BUTTON_HIGHLIGHT = [(255, 232, 170), (190, 170, 140)]
PLUS_FILL = [(255, 246, 214), (176, 160, 136)]
PLUS_OUTLINE = [(104, 46, 8), (64, 52, 40)]
PLUS_ARM = 5.6
PLUS_THICKNESS = 2.2


def to_big(logical):
    return int(round(logical * SCALE * SUPERSAMPLE))


def shrink(image, size):
    return image.resize(size, Image.LANCZOS)


def vertical_fill(size, top, bottom):
    rows = np.linspace(0.0, 1.0, size[1], dtype=np.float32)[:, None, None]
    colours = np.array(top, np.float32) * (1 - rows) + np.array(bottom, np.float32) * rows
    return np.broadcast_to(colours, (size[1], size[0], 3))


def draw_textbox():
    width, height = 170, 21
    big = (to_big(width), to_big(height))
    out = Image.new("RGBA", big, (0, 0, 0, 0))
    draw = ImageDraw.Draw(out)

    rng = np.random.default_rng(GRAIN_SEED)
    plate = vertical_fill(big, PLATE_TOP, PLATE_BOTTOM).copy()
    grain = gaussian_blur(rng.normal(0.0, 1.0, (big[1], big[0])).astype(np.float32), 1.8 * SUPERSAMPLE)
    grain *= GRAIN_STRENGTH / max(float(grain.std()), 1e-6)
    plate += grain[..., None]
    plate_img = Image.fromarray(np.clip(plate, 0, 255).astype(np.uint8), "RGB").convert("RGBA")

    outer = Image.new("L", big, 0)
    ImageDraw.Draw(outer).rounded_rectangle([0, 0, big[0] - 1, big[1] - 1], radius=to_big(BOX_CORNER), fill=255)
    out.paste(plate_img, (0, 0), outer)

    frame = to_big(FRAME_WIDTH)
    for index, colour in enumerate((FRAME_LIGHT, FRAME_DARK)):
        inset = frame * index
        draw.rounded_rectangle([inset, inset, big[0] - 1 - inset, big[1] - 1 - inset],
                               radius=max(to_big(BOX_CORNER) - inset, 2), outline=colour + (255,), width=frame)

    value_left = to_big(DIVIDER_X)
    pad = to_big(VALUE_INSET) + frame * 2
    box = [value_left, pad, big[0] - 1 - pad, big[1] - 1 - pad]
    draw.rounded_rectangle(box, radius=to_big(2.0), fill=VALUE_FILL + (255,))
    draw.rounded_rectangle(box, radius=to_big(2.0), outline=(150, 152, 158, 255), width=max(frame // 2, 1))
    return np.asarray(shrink(out, (width * SCALE, height * SCALE)))


def plus_points(centre_x, centre_y):
    arm, half = PLUS_ARM, PLUS_THICKNESS / 2.0
    return [(centre_x - half, centre_y - arm), (centre_x + half, centre_y - arm), (centre_x + half, centre_y - half),
            (centre_x + arm, centre_y - half), (centre_x + arm, centre_y + half), (centre_x + half, centre_y + half),
            (centre_x + half, centre_y + arm), (centre_x - half, centre_y + arm), (centre_x - half, centre_y + half),
            (centre_x - arm, centre_y + half), (centre_x - arm, centre_y - half), (centre_x - half, centre_y - half)]


def draw_button_cell(state):
    width, height = BUTTON_CELL
    big = (to_big(width), to_big(height))
    image = Image.new("RGBA", big, (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    radius = to_big(BUTTON_CORNER)
    draw.rounded_rectangle([0, 0, big[0] - 1, big[1] - 1], radius=radius, fill=BUTTON_EDGE[state] + (255,))

    inset = to_big(1.0)
    face_top, face_bottom = BUTTON_FACES[state]
    face = Image.fromarray(np.clip(vertical_fill(big, face_top, face_bottom), 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    mask = Image.new("L", big, 0)
    ImageDraw.Draw(mask).rounded_rectangle([inset, inset, big[0] - 1 - inset, big[1] - 1 - inset],
                                           radius=max(radius - inset, 2), fill=255)
    image.paste(face, (0, 0), mask)

    shine = to_big(0.8)
    draw.rounded_rectangle([inset, inset, big[0] - 1 - inset, big[1] - 1 - inset], radius=max(radius - inset, 2),
                           outline=BUTTON_HIGHLIGHT[state] + (255,), width=shine)

    cx, cy = big[0] / 2.0, big[1] / 2.0
    points = [(x * SCALE * SUPERSAMPLE + cx - 0.0, y * SCALE * SUPERSAMPLE + cy - 0.0) for x, y in plus_points(0.0, 0.0)]
    draw.polygon(points, fill=PLUS_FILL[state] + (255,), outline=PLUS_OUTLINE[state] + (255,), width=to_big(0.7))
    return np.asarray(shrink(image, (width * SCALE, height * SCALE)))


def draw_plus_button():
    return np.concatenate([draw_button_cell(0), draw_button_cell(1)], axis=0)


def apply(name, master):
    key = name.lower()
    if key == TEXTBOX:
        return draw_textbox()
    if key == PLUS_BUTTON:
        return draw_plus_button()
    return master
