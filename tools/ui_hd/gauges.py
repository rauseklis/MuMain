"""Redraws the AG and SD tubes as smooth cylinders.

The originals are 16 x 39 JPEGs of a shaded cylinder with a lot of noise. Enlarging them with a network turns the noise
into hard vertical bands, so the tubes look blocky. Here the shading is measured from the original (the average colour
of each column and each row) and drawn again as a smooth gradient at 4x.
"""
import numpy as np

SCALE = 4
SPECS = {"newui_menu_ag.ozj", "newui_menu_sd.ozj"}
ROW_VARIATION = 0.6


def interpolate_profile(values, size):
    """Cubic-ish interpolation of per-pixel colours (n, 3) to `size` samples (pixel centres aligned)."""
    count = len(values)
    positions = (np.arange(size, dtype=np.float32) + 0.5) * count / size - 0.5
    positions = np.clip(positions, 0, count - 1)
    padded = np.pad(values, ((1, 2), (0, 0)), mode="edge")
    base = np.floor(positions).astype(int)
    t = (positions - base)[:, None]
    p0, p1, p2, p3 = padded[base], padded[base + 1], padded[base + 2], padded[base + 3]
    return 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t ** 2 + (-p0 + 3 * p1 - 3 * p2 + p3) * t ** 3)


LIT_THRESHOLD = 40


def tube_box(rgb):
    """Bounding box (c0, c1, r0, r1), inclusive, of the lit pixels of the original tube."""
    lit = rgb.mean(axis=2) > LIT_THRESHOLD
    columns = np.nonzero(lit.mean(axis=0) > 0.5)[0]
    rows = np.nonzero(lit.mean(axis=1) > 0.5)[0]
    return columns.min(), columns.max(), rows.min(), rows.max()


SMOOTHING_KERNEL = np.array([0.06, 0.12, 0.2, 0.24, 0.2, 0.12, 0.06], np.float32)


def smooth_columns(columns):
    """Blends the hard vertical bands of the original shading into one smooth cylinder gradient."""
    pad = len(SMOOTHING_KERNEL) // 2
    padded = np.pad(columns, ((pad, pad), (0, 0)), mode="edge")
    return np.stack([np.convolve(padded[:, c], SMOOTHING_KERNEL, mode="valid") for c in range(columns.shape[1])], axis=1)


def draw_tube(original):
    """original: uint8 RGBA (h, w, 4). Returns the 4x tube: black outside the tube, smooth shading inside."""
    rgb = original[..., :3].astype(np.float32)
    height, width = rgb.shape[:2]
    c0, c1, r0, r1 = tube_box(rgb)
    inner = rgb[r0:r1 + 1, c0:c1 + 1]
    columns = smooth_columns(inner.mean(axis=0))
    column_profile = np.clip(interpolate_profile(columns, inner.shape[1] * SCALE), 0, 255)

    rows = inner.mean(axis=(1, 2))
    rows = np.convolve(np.pad(rows, 2, mode="edge"), np.ones(5) / 5.0, mode="valid")
    row_profile = interpolate_profile(rows[:, None], inner.shape[0] * SCALE)[:, 0]
    row_factor = 1.0 + ROW_VARIATION * (row_profile / max(row_profile.mean(), 1.0) - 1.0)

    tube = column_profile[None, :, :] * row_factor[:, None, None]
    out = np.zeros((height * SCALE, width * SCALE, 4), np.uint8)
    out[..., 3] = 255
    y0, x0 = r0 * SCALE, c0 * SCALE
    out[y0:y0 + tube.shape[0], x0:x0 + tube.shape[1], :3] = np.clip(np.rint(tube), 0, 255).astype(np.uint8)
    return out


def apply(name, master, original):
    if name.lower() not in SPECS:
        return master
    return draw_tube(original)
