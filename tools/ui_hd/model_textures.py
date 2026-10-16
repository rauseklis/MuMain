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
from mu_image import image_size, read_texture
from upscale import downscale_rgba, upscale_rgba, upscale_rgba_straight

SCALES = (4, 2)
BULK_SCALES = (2,)
MASTER_SCALE = 4
# folder -> file name patterns: textures that get 4x and 2x (the mounts that are looked at closely)
MODEL_TEXTURES = {
    "Skill": ["dk*", "fenril*"],
    "Item": ["fenril*"],
}
# folder -> every other texture of the folder, 2x only (wings, weapons, armour, other mounts and effects)
BULK_FOLDERS = ("Item", "Skill", "Player", "Monster", "NPC")
# Numbered folders: World1.., Object1.. (the terrain and the scenery of every map). Their textures repeat across surfaces.
NUMBERED_FAMILIES = ("World", "Object")
# Textures above this size are already high resolution (some Monster maps are 1500 px) and are left alone.
MAX_BULK_SIDE = 512
# Tiny textures (potions, jewels, fruits, small effects) only get blurred by an enlargement and are drawn at icon size, so the
# bulk run leaves them alone.
MIN_BULK_SIDE = 33
TEXTURE_EXTENSIONS = (".ozj", ".ozt")


def source_files(folder, patterns=None):
    """One source file per texture name. Names that exist as both .ozj and .ozt are skipped (see below)."""
    chosen = {}
    twins = {}
    patterns = patterns or MODEL_TEXTURES[folder]
    for pattern in patterns:
        for path in glob.glob(os.path.join(DATA_DIR, folder, pattern)):
            stem, extension = os.path.splitext(os.path.basename(path).lower())
            if extension in TEXTURE_EXTENSIONS:
                twins[stem] = twins.get(stem, 0) + 1
    for pattern in patterns:
        for path in sorted(glob.glob(os.path.join(DATA_DIR, folder, pattern))):
            stem, extension = os.path.splitext(os.path.basename(path))
            if extension.lower() not in TEXTURE_EXTENSIONS:
                continue
            if twins[stem.lower()] > 1:
                continue  # a .ozj and a .ozt with the same name share one HD path, and the game may load either: skip both
            chosen[stem.lower()] = path
    return list(chosen.values())


def output_path(folder, scale, path, keep_extension=False):
    base = os.path.basename(path)
    if keep_extension:
        # The game asks for "x.jpg" / "x.tga" (it swaps the extension to .OZJ / .OZT itself), so the HD name uses those.
        stem, extension = os.path.splitext(base)
        name = stem + "." + {".ozj": "jpg", ".ozt": "tga"}[extension.lower()] + ".png"
    else:
        name = os.path.splitext(base)[0] + ".png"
    return os.path.join(DATA_DIR, f"{folder}@{scale}x", name)


def enlarge(textures, tiling=False):
    results = esrgan.enlarge_many({str(i): rgba for i, (_, rgba, _) in enumerate(textures)}, NETWORK_MODEL, tiling=tiling)
    masters = []
    for i, (path, rgba, kind) in enumerate(textures):
        master = results[str(i)]
        mae, alpha = drift(rgba, master)
        has_cutout = kind == "ozt" and bool((rgba[..., 3] < 255).any())
        if has_cutout:
            master = upscale_rgba_straight(rgba, MASTER_SCALE)
        elif mae > MAX_MAE or alpha > MAX_ALPHA_ERROR:
            master = upscale_rgba(rgba, MASTER_SCALE, jpeg_source=(kind == "ozj"))
            print("  plain enlargement (cut-out or drift):", os.path.basename(path))
        masters.append(master)
    return masters


BATCH_SIZE = int(os.environ.get("UI_HD_BATCH", "40"))  # lower it when several runs share the machine's memory


def write_outputs(folder, textures, masters, scales, keep_extension=False):
    for (path, _, _), master in zip(textures, masters):
        for scale in scales:
            reduced = master if scale == MASTER_SCALE else downscale_rgba(master, MASTER_SCALE // scale)
            save_png(output_path(folder, scale, path, keep_extension), reduced)
        print("ok", folder, os.path.basename(path))


def build(folder, paths, scales, tiling=False, keep_extension=False):
    for start in range(0, len(paths), BATCH_SIZE):
        textures = []
        for path in paths[start:start + BATCH_SIZE]:
            try:
                textures.append((path,) + read_texture(path))
            except ValueError as error:  # e.g. a 24-bit .OZT, which the game itself cannot load
                print("skipped (unreadable):", os.path.basename(path), error, flush=True)
        if textures:
            write_outputs(folder, textures, enlarge(textures, tiling), scales, keep_extension)


def numbered_folders():
    names = sorted(name for name in os.listdir(DATA_DIR)
                   if os.path.isdir(os.path.join(DATA_DIR, name)) and "@" not in name
                   and any(name.startswith(family) and name[len(family):].isdigit() for family in NUMBERED_FAMILIES))
    return names


FOUR_X_SCALES = (4, 2)


def four_x_set(folder, count):
    """The `count` largest textures of a folder (the ones close to the camera are the big ones), skipping twin names."""
    paths = [p for p in source_files(folder, ["*"]) if MIN_BULK_SIDE <= max(image_size(p)) <= MAX_BULK_SIDE]
    paths.sort(key=lambda p: (-max(image_size(p)), p))
    done = {os.path.splitext(os.path.basename(p))[0].lower() for p in glob.glob(os.path.join(DATA_DIR, f"{folder}@4x", "*.png"))}
    return [p for p in paths[:count] if os.path.splitext(os.path.basename(p))[0].lower() not in done]


def twin_files(folder):
    """The .ozj/.ozt files of a folder whose name is shared with a file of the other kind (different pictures)."""
    by_stem = {}
    for path in glob.glob(os.path.join(DATA_DIR, folder, "*")):
        stem, extension = os.path.splitext(os.path.basename(path).lower())
        if extension in TEXTURE_EXTENSIONS:
            by_stem.setdefault(stem, []).append(path)
    return [p for paths in by_stem.values() if len(paths) > 1 for p in sorted(paths)]


def build_twins():
    folders = [f for f in sorted(os.listdir(DATA_DIR)) if os.path.isdir(os.path.join(DATA_DIR, f)) and "@" not in f]
    eligible = set(MODEL_TEXTURES) | set(BULK_FOLDERS) | set(numbered_folders())
    for folder in folders:
        if folder not in eligible:
            continue
        done = {os.path.basename(p).lower() for p in glob.glob(os.path.join(DATA_DIR, f"{folder}@2x", "*.png"))}
        paths = [p for p in twin_files(folder) if MIN_BULK_SIDE <= max(image_size(p)) <= MAX_BULK_SIDE
                 and os.path.basename(output_path(folder, 2, p, True)).lower() not in done]
        if paths:
            print("folder", folder, len(paths), "twin textures", flush=True)
            build(folder, paths, BULK_SCALES, tiling=folder.startswith(("World", "Object")), keep_extension=True)


def build_numbered(prefix):
    for folder in numbered_folders():
        if not folder.startswith(prefix):
            continue
        done = {os.path.splitext(os.path.basename(p))[0].lower() for p in glob.glob(os.path.join(DATA_DIR, f"{folder}@2x", "*.png"))}
        paths = [p for p in source_files(folder, ["*"])
                 if os.path.splitext(os.path.basename(p))[0].lower() not in done and MIN_BULK_SIDE <= max(image_size(p)) <= MAX_BULK_SIDE]
        print("folder", folder, len(paths), "textures", flush=True)
        build(folder, paths, BULK_SCALES, tiling=True)


def main():
    if "--twins" in sys.argv:
        build_twins()
        return 0
    if "--world" in sys.argv:
        build_numbered("World")
        return 0
    if "--objects" in sys.argv:
        build_numbered("Object")
        return 0
    if "--four" in sys.argv:
        for argument in sys.argv[sys.argv.index("--four") + 1:]:
            folder, _, count = argument.partition(":")
            build(folder, four_x_set(folder, int(count or 40)), FOUR_X_SCALES)
        return 0
    bulk = "--bulk" in sys.argv
    for folder in (BULK_FOLDERS if bulk else MODEL_TEXTURES):
        if bulk:
            done = {os.path.splitext(os.path.basename(p))[0].lower() for p in glob.glob(os.path.join(DATA_DIR, f"{folder}@2x", "*.png"))}
            paths = [p for p in source_files(folder, ["*"])
                     if os.path.splitext(os.path.basename(p))[0].lower() not in done and MIN_BULK_SIDE <= max(image_size(p)) <= MAX_BULK_SIDE]
            build(folder, paths, BULK_SCALES)
        else:
            build(folder, source_files(folder), SCALES)
    return 0


if __name__ == "__main__":
    sys.exit(main())
