"""Builds the HD tiers for interface textures.

  python tools/ui_hd/generate.py                    core windows: 4x masters and 2x tier
  python tools/ui_hd/generate.py --set rest         every other interface texture: 2x tier only (keeps the repository small)
  python tools/ui_hd/generate.py --only newui_item_back01.OZT
  python tools/ui_hd/generate.py --model realesrgan-x4plus   bolder, crisper look (default: realesr-animevideov3)
  python tools/ui_hd/generate.py --method lanczos   no network: plain enlargement + sharpening (no extra tools needed)

Masters are Data/Interface@4x/<same path>.png; Data/Interface@2x is always reduced from a 4x result.
The learned method (default) needs Real-ESRGAN, see esrgan.py.
"""
import argparse
import os
import sys

import numpy as np
from PIL import Image

from core_set import CORE_SET
from mu_image import read_texture
from survey import survey
from upscale import add_grain, downscale_rgba, upscale_rgba

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA_DIR = os.path.join(REPO_ROOT, "src", "bin", "Data")
MASTER_SCALE = 4
CORE_TIERS = (4, 2)
REST_TIERS = (2,)
GRAIN_TEXTURES = {"newui_msgbox_back.OZJ"}
LANCZOS_ONLY = GRAIN_TEXTURES
MIN_NETWORK_SIDE = 16
DEFAULT_NETWORK_MODEL = "realesr-animevideov3"
NETWORK_MODEL = os.environ.get("UI_HD_MODEL", DEFAULT_NETWORK_MODEL)
BATCH_SIZE = 48
MAX_MAE = 8.0
MAX_ALPHA_ERROR = 10.0


def tier_path(relative, scale):
    stem = os.path.splitext(relative)[0]
    return os.path.join(DATA_DIR, f"Interface@{scale}x", stem + ".png")


def source_path(relative):
    return os.path.join(DATA_DIR, "Interface", relative)


def stem_key(relative):
    return os.path.splitext(relative)[0].lower()


def all_textures():
    """Textures that have an unambiguous HD path. Two files that differ only by extension (.OZJ and .OZT with the
    same stem) would share one PNG path, so neither gets HD art."""
    rows = [row[0] for row in survey()]
    counts = {}
    for relative in rows:
        counts[stem_key(relative)] = counts.get(stem_key(relative), 0) + 1
    return [relative for relative in rows if counts[stem_key(relative)] == 1]


def drift(original, master):
    reduced = downscale_rgba(master, MASTER_SCALE)
    a, b = original.astype(np.float32), reduced.astype(np.float32)
    mae = float(np.mean(np.abs(a[..., :3] * (a[..., 3:] / 255.0) - b[..., :3] * (b[..., 3:] / 255.0))))
    alpha = float(np.mean(np.abs(a[..., 3] - b[..., 3])))
    return mae, alpha


def within_tolerance(original, master):
    mae, alpha = drift(original, master)
    return mae <= MAX_MAE and alpha <= MAX_ALPHA_ERROR


def remove_tiers(relative):
    for scale in (4, 2):
        path = tier_path(relative, scale)
        if os.path.exists(path):
            os.remove(path)


def uses_network(relative, rgba, method):
    if method != "esrgan" or relative in LANCZOS_ONLY:
        return False
    return min(rgba.shape[:2]) >= MIN_NETWORK_SIDE


def enlarge_lanczos(relative, rgba, kind):
    hd = upscale_rgba(rgba, MASTER_SCALE, jpeg_source=(kind == "ozj"))
    return add_grain(hd, MASTER_SCALE) if relative in GRAIN_TEXTURES else hd


def save_png(path, rgba):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    Image.fromarray(rgba).save(path, optimize=True)


def write_tiers(relative, master, tiers):
    for scale in tiers:
        reduced = master if scale == MASTER_SCALE else downscale_rgba(master, MASTER_SCALE // scale)
        save_png(tier_path(relative, scale), reduced)


def network_masters(loaded, method):
    """Tries the chosen model first and the gentler default for what drifts too far. Returns {name: (master, model)}."""
    pending = {name: rgba for name, (rgba, _) in loaded.items() if uses_network(name, rgba, method)}
    masters = {}
    for model in dict.fromkeys([NETWORK_MODEL, DEFAULT_NETWORK_MODEL]):
        if not pending:
            break
        import esrgan
        enlarged = esrgan.enlarge_many({str(i): rgba for i, rgba in enumerate(pending.values())}, model)
        for i, name in enumerate(list(pending)):
            if within_tolerance(pending[name], enlarged[str(i)]):
                masters[name] = (enlarged[str(i)], model)
        pending = {name: rgba for name, rgba in pending.items() if name not in masters}
    return masters


def build_batch(names, tiers, method):
    loaded = {name: read_texture(source_path(name)) for name in names}
    masters = network_masters(loaded, method)
    for name, (rgba, kind) in loaded.items():
        master, label = masters.get(name, (None, "lanczos"))
        if master is None:
            master = enlarge_lanczos(name, rgba, kind)
        if not within_tolerance(rgba, master):
            remove_tiers(name)
            print("skipped", name, "(drifts too far from the original)")
            continue
        write_tiers(name, master, tiers)
        print("ok", name, f"({label})")


def select(args):
    core = set(CORE_SET)
    if args.only and args.only not in all_textures():
        raise SystemExit(f"{args.only} has no unambiguous HD path")
    if args.only:
        return [args.only], CORE_TIERS if args.only in core else REST_TIERS
    if args.set == "rest":
        return [name for name in all_textures() if name not in core], REST_TIERS
    return list(CORE_SET), CORE_TIERS


def main():
    global NETWORK_MODEL
    parser = argparse.ArgumentParser()
    parser.add_argument("--only")
    parser.add_argument("--set", choices=("core", "rest"), default="core")
    parser.add_argument("--method", choices=("esrgan", "lanczos"), default="esrgan")
    parser.add_argument("--model", default=NETWORK_MODEL)
    args = parser.parse_args()
    NETWORK_MODEL = args.model
    names, tiers = select(args)
    for start in range(0, len(names), BATCH_SIZE):
        build_batch(names[start:start + BATCH_SIZE], tiers, args.method)


if __name__ == "__main__":
    sys.exit(main())
