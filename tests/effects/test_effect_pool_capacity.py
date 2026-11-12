"""The shared effect and joint pools must stay large enough for sustained casting.

A creation that finds no free slot is silently dropped, so an undersized pool shows up in the game as a cast
animation without its projectile. Measured demand (control-socket client, 1920x1080, nonstop casting): Elf Multi Shot
needs about 470 effects at once, Heal / Greater Defense spam about 870 joints.
"""

from pathlib import Path
import re
import sys

root = Path(sys.argv[1])
defines = (root / "src/source/Core/Globals/_define.h").read_text(encoding="utf-8", errors="replace")


def value(name: str) -> int:
    match = re.search(r"^#define\s+" + name + r"\s+(\d+)\s*$", defines, re.MULTILINE)
    assert match, name + " is not defined as a plain number in _define.h"
    return int(match.group(1))


assert value("MAX_EFFECTS") >= 1000, "MAX_EFFECTS must cover sustained Multi Shot (about 470) with headroom for crowds"
assert value("MAX_JOINTS") >= 2000, "MAX_JOINTS must cover sustained buff spam (about 870) with headroom for crowds"

print("PASS: effect and joint pools are sized for sustained casting")
