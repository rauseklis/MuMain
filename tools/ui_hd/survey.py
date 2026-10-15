"""Lists every interface texture: format, size, padded size and which window core set it belongs to.

  python tools/ui_hd/survey.py [--csv out.csv]
"""
import argparse
import csv
import os
import sys

from mu_image import image_size, padded_size

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
INTERFACE_DIR = os.path.join(REPO_ROOT, "src", "bin", "Data", "Interface")
TEXTURE_EXTENSIONS = (".ozt", ".ozj")


def survey(interface_dir=INTERFACE_DIR):
    rows = []
    for folder, _, files in os.walk(interface_dir):
        for name in sorted(files):
            if not name.lower().endswith(TEXTURE_EXTENSIONS):
                continue
            path = os.path.join(folder, name)
            width, height = image_size(path)
            pad_w, pad_h = padded_size(width, height)
            relative = os.path.relpath(path, interface_dir).replace("\\", "/")
            rows.append((relative, name.lower().rsplit(".", 1)[1], width, height, pad_w, pad_h))
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv")
    args = parser.parse_args()
    rows = survey()
    texels = {k: sum(r[4] * r[5] for r in rows) * k * k for k in (1, 2, 4)}
    print(f"{len(rows)} textures")
    for scale, count in texels.items():
        print(f"  {scale}x padded RGBA: {count * 4 / 1048576:.1f} MiB")
    if args.csv:
        with open(args.csv, "w", newline="") as handle:
            writer = csv.writer(handle)
            writer.writerow(["path", "format", "width", "height", "padded_width", "padded_height"])
            writer.writerows(rows)


if __name__ == "__main__":
    sys.exit(main())
