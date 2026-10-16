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


def apply(name, master):
    if name in BUTTONS:
        return remove_edge_specks(master)
    if name != SPEC:
        return master
    out = master.copy()
    source = master[:, CLEAN_X * SCALE + SCALE // 2].copy()
    for start, end in DIVIDERS:
        for column in range(start * SCALE, end * SCALE):
            out[:, column] = source
    return out
