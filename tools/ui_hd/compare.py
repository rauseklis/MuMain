"""Fidelity check: reduces an HD texture by its scale and compares it with the original.

  python tools/ui_hd/compare.py <original.OZT|OZJ> <hd.png> [--sheet out.png]
Reports mean absolute error (0-255) and SSIM on premultiplied colour, plus a side-by-side sheet.
"""
import argparse
import sys

import numpy as np
from PIL import Image

from mu_image import read_texture
from upscale import downscale_rgba, gaussian_blur

SSIM_SIGMA = 1.5
SSIM_C1 = (0.01 * 255) ** 2
SSIM_C2 = (0.03 * 255) ** 2


def ssim(a, b):
    a = a.astype(np.float64)
    b = b.astype(np.float64)
    scores = []
    for channel in range(a.shape[2]):
        x, y = a[..., channel], b[..., channel]
        mu_x, mu_y = gaussian_blur(x, SSIM_SIGMA), gaussian_blur(y, SSIM_SIGMA)
        var_x = gaussian_blur(x * x, SSIM_SIGMA) - mu_x ** 2
        var_y = gaussian_blur(y * y, SSIM_SIGMA) - mu_y ** 2
        cov = gaussian_blur(x * y, SSIM_SIGMA) - mu_x * mu_y
        num = (2 * mu_x * mu_y + SSIM_C1) * (2 * cov + SSIM_C2)
        den = (mu_x ** 2 + mu_y ** 2 + SSIM_C1) * (var_x + var_y + SSIM_C2)
        scores.append(float(np.mean(num / den)))
    return float(np.mean(scores))


def premultiplied(rgba):
    alpha = rgba[..., 3:4].astype(np.float64) / 255.0
    return np.concatenate([rgba[..., :3] * alpha, rgba[..., 3:4]], axis=2)


def compare(original, hd):
    height, width = original.shape[:2]
    scale = hd.shape[1] // width
    if (hd.shape[0], hd.shape[1]) != (height * scale, width * scale) or scale < 1:
        raise ValueError(f"size mismatch: original {width}x{height}, HD {hd.shape[1]}x{hd.shape[0]}")
    reduced = downscale_rgba(hd, scale)
    a, b = premultiplied(original), premultiplied(reduced)
    mae = float(np.mean(np.abs(a - b)))
    return scale, mae, ssim(a, b)


def side_by_side(original, hd, scale):
    height, width = hd.shape[:2]
    left = Image.fromarray(original).resize((width, height), Image.NEAREST)
    right = Image.fromarray(hd)
    sheet = Image.new("RGBA", (width * 2 + 8, height), (255, 0, 255, 255))
    sheet.alpha_composite(left, (0, 0))
    sheet.alpha_composite(right, (width + 8, 0))
    return sheet.convert("RGB")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("original")
    parser.add_argument("hd")
    parser.add_argument("--sheet")
    args = parser.parse_args()
    original, _ = read_texture(args.original)
    hd = np.asarray(Image.open(args.hd).convert("RGBA"))
    scale, mae, score = compare(original, hd)
    print(f"{scale}x  MAE {mae:.2f}  SSIM {score:.4f}")
    if args.sheet:
        side_by_side(original, hd, scale).save(args.sheet)


if __name__ == "__main__":
    sys.exit(main())
