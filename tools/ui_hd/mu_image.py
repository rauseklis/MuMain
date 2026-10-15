"""Reads and writes the client's interface texture formats (OZT = 32-bit TGA with a 4-byte prefix, OZJ = JPEG with a 24-byte header)."""
import io
import os
import struct

import numpy as np
from PIL import Image

OZT_PREFIX_END = 12
OZT_HEADER_SIZE = 22
OZJ_HEADER_SIZE = 24
OZT_FOOTER = bytes(8) + b"TRUEVISION-XFILE.\x00"
BITS_PER_PIXEL = 32


def next_power_of_two(value, limit=1024):
    result = 1
    while result < value and result < limit:
        result <<= 1
    return min(result, limit)


def read_ozt(path):
    """Returns an RGBA uint8 array (height, width, 4)."""
    data = open(path, "rb").read()
    width, height = struct.unpack_from("<hh", data, 16)
    bits = data[20]
    if bits != BITS_PER_PIXEL:
        raise ValueError(f"{path}: {bits} bits per pixel")
    body = np.frombuffer(data, np.uint8, width * height * 4, OZT_HEADER_SIZE).reshape(height, width, 4)
    rgba = body[::-1, :, [2, 1, 0, 3]]
    return np.ascontiguousarray(rgba)


def write_ozt(path, rgba):
    height, width = rgba.shape[:2]
    header = bytearray(bytes([0, 0, 2]) + bytes(5) + bytes(2) + bytes(2) + bytes(4))
    header += struct.pack("<hh", width, height) + bytes([BITS_PER_PIXEL, 0])
    body = np.ascontiguousarray(rgba[::-1, :, [2, 1, 0, 3]]).tobytes()
    with open(path, "wb") as handle:
        handle.write(bytes(header) + body + OZT_FOOTER)


def read_ozj(path):
    """Returns an RGB uint8 array (height, width, 3)."""
    data = open(path, "rb").read()[OZJ_HEADER_SIZE:]
    return np.asarray(Image.open(io.BytesIO(data)).convert("RGB"))


def read_ozj_header(path):
    data = open(path, "rb").read()
    return data[:OZJ_HEADER_SIZE]


def read_texture(path):
    """Returns (rgba array, kind) where kind is 'ozt' or 'ozj'. JPEG sources get an opaque alpha channel."""
    ext = os.path.splitext(path)[1].lower()
    if ext == ".ozt":
        return read_ozt(path), "ozt"
    if ext == ".ozj":
        rgb = read_ozj(path)
        alpha = np.full(rgb.shape[:2] + (1,), 255, np.uint8)
        return np.concatenate([rgb, alpha], axis=2), "ozj"
    raise ValueError(f"unsupported texture extension: {path}")


def image_size(path):
    ext = os.path.splitext(path)[1].lower()
    if ext == ".ozt":
        with open(path, "rb") as handle:
            head = handle.read(OZT_HEADER_SIZE)
        return struct.unpack_from("<hh", head, 16)
    data = open(path, "rb").read()[OZJ_HEADER_SIZE:]
    return Image.open(io.BytesIO(data)).size


def padded_size(width, height):
    return next_power_of_two(width), next_power_of_two(height)
