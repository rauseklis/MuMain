"""Same-look upscaler for interface art: premultiplied Lanczos, halo-free sharpening, crisp alpha edges.

No learned model: it only enlarges what the original shows. Every pixel position scales exactly by the factor.
"""
import numpy as np
from PIL import Image

SHARPEN_SIGMA_PER_SCALE = 0.55
SHARPEN_AMOUNT = 0.9
ALPHA_EDGE_GAIN = 1.6
JPEG_DENOISE_SIGMA = 0.55
LIMIT_RADIUS_PER_SCALE = 0.75
GRAIN_STRENGTH = 2.2
GRAIN_SEED = 20261014


def _gaussian_kernel(sigma):
    radius = max(1, int(np.ceil(sigma * 3)))
    xs = np.arange(-radius, radius + 1)
    kernel = np.exp(-(xs ** 2) / (2 * sigma * sigma))
    return kernel / kernel.sum()


def _convolve_axis(channel, kernel, axis):
    radius = len(kernel) // 2
    pad = [(0, 0)] * channel.ndim
    pad[axis] = (radius, radius)
    padded = np.pad(channel, pad, mode="edge")
    out = np.zeros_like(channel)
    length = channel.shape[axis]
    for offset, weight in enumerate(kernel):
        index = [slice(None)] * channel.ndim
        index[axis] = slice(offset, offset + length)
        out += weight * padded[tuple(index)]
    return out


def gaussian_blur(channel, sigma):
    kernel = _gaussian_kernel(sigma)
    return _convolve_axis(_convolve_axis(channel, kernel, 0), kernel, 1)


def _local_extrema(channel, radius):
    low = channel.copy()
    high = channel.copy()
    height, width = channel.shape
    padded_low = np.pad(channel, radius, mode="edge")
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            window = padded_low[radius + dy:radius + dy + height, radius + dx:radius + dx + width]
            low = np.minimum(low, window)
            high = np.maximum(high, window)
    return low, high


def _lanczos(plane, scale):
    height, width = plane.shape
    image = Image.fromarray(plane.astype(np.float32), mode="F")
    return np.asarray(image.resize((width * scale, height * scale), Image.LANCZOS), dtype=np.float32)


def _bicubic(plane, scale):
    height, width = plane.shape
    image = Image.fromarray(plane.astype(np.float32), mode="F")
    return np.asarray(image.resize((width * scale, height * scale), Image.BICUBIC), dtype=np.float32)


def _sharpen_limited(channel, base, scale, amount, sigma):
    """Unsharp mask, clamped to the neighbourhood range of a smooth upscale so it cannot add halos."""
    blurred = gaussian_blur(channel, sigma)
    sharp = channel + amount * (channel - blurred)
    low, high = _local_extrema(base, max(1, int(round(LIMIT_RADIUS_PER_SCALE * scale))))
    return np.clip(sharp, low, high)


def _denoise(plane):
    return gaussian_blur(plane, JPEG_DENOISE_SIGMA)


def upscale_rgba(rgba, scale, jpeg_source=False, sharpen=True):
    """rgba: uint8 (h, w, 4). Returns uint8 (h*scale, w*scale, 4)."""
    data = rgba.astype(np.float32)
    alpha = data[..., 3] / 255.0
    premultiplied = [data[..., i] * alpha for i in range(3)]
    planes = [] 
    for channel in premultiplied:
        source = _denoise(channel) if jpeg_source else channel
        base = _bicubic(source, scale)
        lanczos = _lanczos(source, scale)
        result = lanczos
        if sharpen:
            result = _sharpen_limited(lanczos, base, scale, SHARPEN_AMOUNT, SHARPEN_SIGMA_PER_SCALE * scale)
        planes.append(result)
    alpha_up = _lanczos(alpha, scale)
    alpha_up = np.clip(alpha_up, 0.0, 1.0)
    if sharpen and np.any((alpha > 0) & (alpha < 1)):
        alpha_up = np.clip((alpha_up - 0.5) * ALPHA_EDGE_GAIN + 0.5, 0.0, 1.0)
    out = np.zeros(alpha_up.shape + (4,), np.float32)
    safe = np.maximum(alpha_up, 1e-4)
    for i, plane in enumerate(planes):
        color = np.where(alpha_up > 1e-3, plane / safe, 0.0)
        out[..., i] = np.clip(color, 0, 255)
    out[..., 3] = alpha_up * 255.0
    return np.clip(np.rint(out), 0, 255).astype(np.uint8)


def add_grain(rgba, scale, strength=GRAIN_STRENGTH):
    """Re-synthesises fine texture lost to enlargement: seeded, luminance-only noise over opaque pixels."""
    rng = np.random.default_rng(GRAIN_SEED)
    height, width = rgba.shape[:2]
    noise = rng.normal(0.0, 1.0, (height, width)).astype(np.float32)
    noise = gaussian_blur(noise, 0.5 * scale ** 0.5)
    noise *= strength / max(noise.std(), 1e-6)
    out = rgba.astype(np.float32)
    mask = (out[..., 3] > 250)[..., None]
    out[..., :3] += np.where(mask, noise[..., None], 0.0)
    return np.clip(np.rint(out), 0, 255).astype(np.uint8)


def downscale_rgba(rgba, scale):
    """Box-filter reduction used for tier generation and for comparisons."""
    if scale == 1:
        return rgba
    height, width = rgba.shape[:2]
    data = rgba.astype(np.float32)
    alpha = data[..., 3:4] / 255.0
    premult = np.concatenate([data[..., :3] * alpha, data[..., 3:4]], axis=2)
    blocks = premult.reshape(height // scale, scale, width // scale, scale, 4).mean(axis=(1, 3))
    a = blocks[..., 3:4] / 255.0
    color = np.where(a > 1e-4, blocks[..., :3] / np.maximum(a, 1e-4), 0.0)
    return np.clip(np.rint(np.concatenate([color, blocks[..., 3:4]], axis=2)), 0, 255).astype(np.uint8)
