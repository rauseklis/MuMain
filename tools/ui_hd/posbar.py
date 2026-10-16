"""Removes the bright dividers between the three buttons at the right end of the position bar.

Minimap_positionC_ExpHunt is the right end of the bar: the end of the ornament, three framed button slots separated by
vertical dividers, then the right cap. The three buttons carry their own dark frames, so the dividers showed as bright
seams between them. the slot area between the buttons is painted over with the plain bar (black with the steel top
and bottom lines); the divider that ends the ornament and the one that starts the cap stay.
"""
import numpy as np

BUTTONS = {
    "MacroUI/MacroUI_Setup.OZT",
    "MacroUI/MacroUI_Start.OZT",
    "MacroUI/MacroUI_Stop.OZT",
    "MacroUI/MacroUI_ExpHuntStart.OZT",
    "MacroUI/MacroUI_ExpHuntStop.OZT",
}
SPECK_COLUMNS = (16, 17)  # logical pixel columns on the right edge of the 18 pixel wide buttons
SPECK_LIGHTER_BY = 90     # a pixel this much brighter (sum of RGB) than the one 1 pixel to its left is a stray speck

SCALE = 4
SPEC = "MacroUI/Minimap_positionC_ExpHunt.OZT"
DIVIDERS = ((20, 70),)  # logical pixel columns [from, to) of the slot area between the first and the last divider
CLEAN_X = 45                     # logical pixel column of plain bar inside the middle slot, copied over the dividers


def remove_edge_specks(master):
    """The play and stop buttons carry a few bright single pixels on their right edge (they show up as white dots
    between the buttons). Each one is replaced by the pixel a logical pixel to its left."""
    out = master.copy()
    for column in range(SPECK_COLUMNS[0] * SCALE, min(SPECK_COLUMNS[1] * SCALE + SCALE, master.shape[1])):
        neighbour = column - SCALE
        lighter = master[:, column, :3].astype(int).sum(axis=1) - master[:, neighbour, :3].astype(int).sum(axis=1)
        rows = (lighter > SPECK_LIGHTER_BY) & (master[:, column, 3] > 0)
        out[rows, column] = master[rows, neighbour]
    return out


CELL_HEIGHT = 13           # logical pixels: the three states are stacked, 13 rows each
CORNER_ROWS = (1, 12)      # logical rows of the frame inside a state, counted from the top of the state
CORNER_COLUMNS = (1, 16)   # logical columns of the frame
CORNER_BRIGHT = 45         # a corner brighter than this (mean of RGB) is a bevel dot and is made dark
CORNER_DARK = 6


def darken_corners(master):
    """Every button frame has a one pixel bright dot in each corner (the bevel's ends). On a bar of dark frames they read
    as white dots between the buttons, so the dots, and the soft halo the enlargement leaves around them, are made dark
    like the frame around them."""
    out = master.copy()
    height, width = master.shape[:2]
    for state in range(3):
        for row in CORNER_ROWS:
            for column in CORNER_COLUMNS:
                y0 = max((state * CELL_HEIGHT + row - 1) * SCALE, 0)
                y1 = min((state * CELL_HEIGHT + row + 2) * SCALE, height)
                x0 = max((column - 1) * SCALE, 0)
                x1 = min((column + 2) * SCALE, width)
                block = out[y0:y1, x0:x1]
                bright = (block[..., :3].astype(int).mean(axis=2) > CORNER_BRIGHT) & (block[..., 3] > 0)
                # only grey dots: coloured pixels (the orange face) are part of the button
                grey = (block[..., :3].astype(int).max(axis=2) - block[..., :3].astype(int).min(axis=2)) < 40
                block[bright & grey, :3] = CORNER_DARK
    return out


def apply(name, master):
    if name in BUTTONS:
        return darken_corners(remove_edge_specks(master))
    if name != SPEC:
        return master
    out = master.copy()
    source = master[:, CLEAN_X * SCALE + SCALE // 2].copy()
    for start, end in DIVIDERS:
        for column in range(start * SCALE, end * SCALE):
            out[:, column] = source
    return out
