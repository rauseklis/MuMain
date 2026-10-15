"""Writes tools/ui_hd/manifest.csv: every interface texture, its HD status and the window set it belongs to.

  python tools/ui_hd/manifest.py
Status is 'done' when a 4x or 2x PNG exists, 'pending' otherwise. Re-run after generating more textures.
"""
import csv
import os
import sys

from core_set import GROUPS
from generate import DATA_DIR
from survey import survey

MANIFEST_PATH = os.path.join(os.path.dirname(__file__), "manifest.csv")
HD_METHOD = "real-esrgan (merged with enlargement)"
UNASSIGNED_GROUP = "other"
OWNER_BY_PATH = {path.lower(): group for group, paths in GROUPS.items() for path in paths}


def has_hd(relative):
    stem = os.path.splitext(relative)[0]
    return any(os.path.exists(os.path.join(DATA_DIR, f"Interface@{scale}x", stem + ".png")) for scale in (4, 2))


def build_rows():
    rows = []
    for relative, kind, width, height, padded_w, padded_h in survey():
        group = OWNER_BY_PATH.get(relative.lower(), UNASSIGNED_GROUP)
        done = has_hd(relative)
        rows.append([relative, group, kind, width, height, padded_w, padded_h,
                     "done" if done else "pending", HD_METHOD if done else ""])
    return rows


def main():
    rows = sorted(build_rows(), key=lambda row: (row[1] == UNASSIGNED_GROUP, row[1], row[0].lower()))
    with open(MANIFEST_PATH, "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["path", "window_set", "format", "width", "height", "padded_width", "padded_height",
                         "status", "method"])
        writer.writerows(rows)
    done = sum(1 for row in rows if row[7] == "done")
    print(f"{done} done, {len(rows) - done} pending of {len(rows)} -> {MANIFEST_PATH}")


if __name__ == "__main__":
    sys.exit(main())
