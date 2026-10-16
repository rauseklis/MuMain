"""Learned 4x enlargement (Real-ESRGAN ncnn build) with the original's tones kept.

The network invents crisp edges and fine detail, but it also shifts contrast and colour. Each result is therefore merged
with the plain Lanczos enlargement: the fine detail comes from the network, the low-frequency tones from the original,
so reducing the result back down still gives the original picture.

The executable is not part of the repository. Point UI_HD_ESRGAN at realesrgan-ncnn-vulkan.exe (its models folder
must sit beside it), or put the extracted release at tools/ui_hd/.esrgan/.
"""
import os
import shutil
import subprocess
import tempfile

import numpy as np
from PIL import Image

from upscale import gaussian_blur

SCALE = 4
TONE_SIGMA_PER_SCALE = 1.0
ALPHA_TONE_SIGMA_PER_SCALE = 0.75
FILL_SIGMAS = (1.0, 2.0, 4.0, 8.0, 16.0, 32.0)
FILL_EPSILON = 1e-3
MIN_NETWORK_SIDE = 32
DEFAULT_MODEL = "realesrgan-x4plus"
HERE = os.path.dirname(os.path.abspath(__file__))
LOCAL_TOOL = os.path.join(HERE, ".esrgan", "realesrgan-ncnn-vulkan.exe")


def find_executable():
    path = os.environ.get("UI_HD_ESRGAN", LOCAL_TOOL)
    if not os.path.exists(path):
        raise FileNotFoundError(f"Real-ESRGAN executable not found: {path} (set UI_HD_ESRGAN)")
    return path


def fill_transparent_colour(rgb, alpha):
    """Spreads the colours of opaque pixels into transparent ones so the network sees no black halo."""
    weight = (alpha > 0.5).astype(np.float32)
    filled = rgb.copy()
    for sigma in FILL_SIGMAS:
        if weight.min() >= 1.0:
            break
        spread = gaussian_blur(weight, sigma)
        for channel in range(3):
            estimate = gaussian_blur(filled[..., channel] * weight, sigma) / np.maximum(spread, FILL_EPSILON)
            filled[..., channel] = np.where((weight == 0) & (spread > FILL_EPSILON), estimate, filled[..., channel])
        weight = np.maximum(weight, (spread > FILL_EPSILON).astype(np.float32))
    return filled


def merge_tones(detail, base, sigma):
    """Keeps `detail`'s fine structure and `base`'s low-frequency tones."""
    result = np.empty_like(detail)
    for channel in range(detail.shape[2]):
        result[..., channel] = (detail[..., channel] - gaussian_blur(detail[..., channel], sigma)
                                + gaussian_blur(base[..., channel], sigma))
    return result


def lanczos(array, scale):
    height, width = array.shape[:2]
    channels = [np.asarray(Image.fromarray(array[..., i].astype(np.float32), mode="F")
                           .resize((width * scale, height * scale), Image.LANCZOS)) for i in range(array.shape[2])]
    return np.stack(channels, axis=2)


def run_network(images, model=DEFAULT_MODEL):
    """images: {name: uint8 RGB array}. Returns {name: uint8 RGB array at 4x}."""
    executable = find_executable()
    work = tempfile.mkdtemp(prefix="ui_hd_esrgan_")
    source_dir, result_dir = os.path.join(work, "in"), os.path.join(work, "out")
    os.makedirs(source_dir)
    os.makedirs(result_dir)
    try:
        for name, array in images.items():
            Image.fromarray(array).save(os.path.join(source_dir, name + ".png"))
        subprocess.run([executable, "-i", source_dir, "-o", result_dir, "-n", model, "-s", str(SCALE), "-f", "png"],
                       check=True, cwd=os.path.dirname(executable), stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)
        return {name: np.asarray(Image.open(os.path.join(result_dir, name + ".png")).convert("RGB"))
                for name in images}
    finally:
        shutil.rmtree(work, ignore_errors=True)


def prepare(rgba):
    """Returns (colour image for the network, alpha image for the network or None when fully opaque)."""
    alpha = rgba[..., 3].astype(np.float32) / 255.0
    rgb = fill_transparent_colour(rgba[..., :3].astype(np.float32), alpha)
    colour = np.clip(np.rint(rgb), 0, 255).astype(np.uint8)
    if alpha.min() >= 1.0:
        return colour, None
    gray = np.clip(np.rint(alpha * 255.0), 0, 255).astype(np.uint8)
    return colour, np.stack([gray, gray, gray], axis=2)


def combine(rgba, colour_result, alpha_result):
    colour = colour_result.astype(np.float32)
    base = lanczos(np.clip(fill_transparent_colour(rgba[..., :3].astype(np.float32),
                                                   rgba[..., 3].astype(np.float32) / 255.0), 0, 255), SCALE)
    colour = np.clip(merge_tones(colour, base, TONE_SIGMA_PER_SCALE * SCALE), 0, 255)

    alpha_base = lanczos(rgba[..., 3:4].astype(np.float32), SCALE)[..., :1]
    if alpha_result is None:
        alpha = np.full(alpha_base.shape, 255.0, np.float32)
    else:
        alpha_detail = alpha_result.astype(np.float32).mean(axis=2, keepdims=True)
        alpha = np.clip(merge_tones(alpha_detail, alpha_base, ALPHA_TONE_SIGMA_PER_SCALE * SCALE), 0, 255)
    return np.clip(np.rint(np.concatenate([colour, alpha], axis=2)), 0, 255).astype(np.uint8)


def pad_amounts(size):
    """Edge padding that lifts a side to at least MIN_NETWORK_SIDE (the network needs some context to work with)."""
    missing = max(MIN_NETWORK_SIDE - size, 0)
    return missing // 2, missing - missing // 2


TILE_MARGIN = 8  # texels of the opposite edge shown to the network so a repeating texture stays seamless


def enlarge_many(textures, model=DEFAULT_MODEL, tiling=False):
    """textures: {name: uint8 RGBA array}. Returns {name: uint8 RGBA array at 4x} (the original area only)."""
    padded, crops = {}, {}
    for name, rgba in textures.items():
        top, bottom = pad_amounts(rgba.shape[0])
        left, right = pad_amounts(rgba.shape[1])
        if tiling:
            top, bottom, left, right = [max(v, TILE_MARGIN) for v in (top, bottom, left, right)]
        padded[name] = np.pad(rgba, ((top, bottom), (left, right), (0, 0)), mode="wrap" if tiling else "edge")
        crops[name] = (top * SCALE, (top + rgba.shape[0]) * SCALE, left * SCALE, (left + rgba.shape[1]) * SCALE)
    prepared = {name: prepare(rgba) for name, rgba in padded.items()}
    colour_inputs = {name + "_c": colour for name, (colour, _) in prepared.items()}
    alpha_inputs = {name + "_a": alpha for name, (_, alpha) in prepared.items() if alpha is not None}
    colour_results = run_network(colour_inputs, model)
    alpha_results = run_network(alpha_inputs, model) if alpha_inputs else {}
    results = {}
    for name, rgba in padded.items():
        full = combine(rgba, colour_results[name + "_c"], alpha_results.get(name + "_a"))
        y0, y1, x0, x1 = crops[name]
        results[name] = full[y0:y1, x0:x1]
    return results
