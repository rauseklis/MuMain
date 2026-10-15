"""Builds HD versions of a few 3D model textures (Dark Horse, Fenrir) whose originals are only 32 to 128 pixels wide.

  python tools/ui_hd/model_textures.py

3D models address their textures by UV fractions, so a texture with more texels needs no other change. The files go to
Data/<Folder>@4x and Data/<Folder>@2x with the same names, as .png, exactly like the interface textures. The game uses
them for the folders Interface, Skill and Item (see Render/Textures/UiHdTextures.h).
"""
import glob
import os
import sys

import numpy as np
from PIL import Image

import esrgan
from generate import DATA_DIR, MAX_ALPHA_ERROR, MAX_MAE, NETWORK_MODEL, drift, save_png
from mu_image import read_texture
from upscale import downscale_rgba, upscale_rgba

SCALES = (4, 2)
MASTER_SCALE = 4
# folder -> file name patterns (without extension)
MODEL_TEXTURES = {
    "Skill": ["dk*", "fenril*"],
    "Item": ["fenril*"],
}
TEXTURE_EXTENSIONS = (".ozj", ".ozt")


def source_files(folder):
    """One source file per texture name. Names that exist as both .ozj and .ozt are skipped (see below)."""
    chosen = {}
    twins = {}
    for pattern in MODEL_TEXTURES[folder]:
        for path in glob.glob(os.path.join(DATA_DIR, folder, pattern)):
            stem, extension = os.path.splitext(os.path.basename(path).lower())
            if extension in TEXTURE_EXTENSIONS:
                twins[stem] = twins.get(stem, 0) + 1
    for pattern in MODEL_TEXTURES[folder]:
        for path in sorted(glob.glob(os.path.join(DATA_DIR, folder, pattern))):
            stem, extension = os.path.splitext(os.path.basename(path))
            if extension.lower() not in TEXTURE_EXTENSIONS:
                continue
            if twins[stem.lower()] > 1:
                continue  # a .ozj and a .ozt with the same name share one HD path, and the game may load either: skip both
            chosen[stem.lower()] = path
    return list(chosen.values())


def output_path(folder, scale, path):
    stem = os.path.splitext(os.path.basename(path))[0]
    return os.path.join(DATA_DIR, f"{folder}@{scale}x", stem + ".png")


def enlarge(textures):
    results = esrgan.enlarge_many({str(i): rgba for i, (_, rgba, _) in enumerate(textures)}, NETWORK_MODEL)
    masters = []
    for i, (path, rgba, kind) in enumerate(textures):
        master = results[str(i)]
        mae, alpha = drift(rgba, master)
        has_cutout = kind == "ozt" and bool((rgba[..., 3] < 255).any())
        if has_cutout or mae > MAX_MAE or alpha > MAX_ALPHA_ERROR:
            master = upscale_rgba(rgba, MASTER_SCALE, jpeg_source=(kind == "ozj"))
            print("  plain enlargement (cut-out or drift):", os.path.basename(path))
        masters.append(master)
    return masters


def main():
    for folder in MODEL_TEXTURES:
        paths = source_files(folder)
        textures = [(path,) + read_texture(path) for path in paths]
        for (path, _, _), master in zip(textures, enlarge(textures)):
            for scale in SCALES:
                reduced = master if scale == MASTER_SCALE else downscale_rgba(master, MASTER_SCALE // scale)
                save_png(output_path(folder, scale, path), reduced)
            print("ok", folder, os.path.basename(path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
