"""Draws tools/exp_hunt_bar/exp_hunt_bar_preview.png: the old bar and the new one, as they look at a 3x scale (1920 x 1080),
with a short and a very long value. The text uses a stand-in font; only the art and the sizes are real."""
import os
from PIL import Image, ImageDraw, ImageFont

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(REPO, "src", "bin", "Data")
S = 3  # screen pixels per logical pixel
BG = (62, 78, 46, 255)
GOLD = (255, 220, 120, 255)
FONT = ImageFont.truetype(os.path.join(REPO, "src", "bin", "fonts", "LiberationSans-Bold.ttf"), 16)  # 16 px at 1080p


def scaled(path, w, h, box=None):
    img = Image.open(path).convert("RGBA")
    if box:
        img = img.crop(box)
    return img.resize((w * S, h * S), Image.LANCZOS)


def old_bar(text):
    w, h = 220, 20
    out = Image.new("RGBA", (w * S, h * S), BG)
    a = os.path.join(DATA, "Interface@2x", "Minimap_positionA.png")
    b = os.path.join(DATA, "Interface@2x", "Minimap_positionB.png")
    cap = scaled(a, 18, 20, (0, 0, 44, 50))
    mid = scaled(b, w - 36, 20, (0, 0, 52, 50))
    out.alpha_composite(cap, (0, 0))
    out.alpha_composite(mid, (18 * S, 0))
    out.alpha_composite(cap.transpose(Image.FLIP_LEFT_RIGHT), ((w - 18) * S, 0))
    d = ImageDraw.Draw(out)
    tw = d.textlength(text, font=FONT)
    d.text(((w * S - tw) / 2, 3 * S), text, font=FONT, fill=GOLD)
    return out


def new_bar(text):
    atlas = Image.open(os.path.join(DATA, "Interface@4x", "ExpHuntBar", "ExpHuntBar.png")).convert("RGBA")
    k = atlas.width // 64
    def piece(x):
        p = atlas.crop((x * k, 2 * k, (x + 16) * k, (2 + 18) * k))
        return p.resize((16 * S, 18 * S), Image.LANCZOS)
    d0 = ImageDraw.Draw(Image.new("RGBA", (1, 1)))
    textw = max(d0.textlength(text, font=FONT), d0.textlength("Experience Hunter: 9,999,999,999 / min", font=FONT))
    w = max(96, int(round(textw / S)) + 2 * (16 + 4))
    out = Image.new("RGBA", (w * S, 18 * S), BG)
    out.alpha_composite(piece(2), (0, 0))
    mid = piece(42).resize(((w - 32) * S, 18 * S), Image.LANCZOS)
    out.alpha_composite(mid, (16 * S, 0))
    out.alpha_composite(piece(22), ((w - 16) * S, 0))
    d = ImageDraw.Draw(out)
    tw = d.textlength(text, font=FONT)
    d.text(((w * S - tw) / 2, 6 * S - 9), text, font=FONT, fill=GOLD)
    return out


rows = [("Experience Hunter: 0 / min", "short"), ("Experience Hunter: 9,999,999,999 / min", "long")]
tiles = []
for text, _ in rows:
    tiles.append(("before", old_bar(text)))
    tiles.append(("after", new_bar(text)))
W = max(t.width for _, t in tiles) + 40
H = sum(t.height + 36 for _, t in tiles) + 20
sheet = Image.new("RGBA", (W, H), BG)
d = ImageDraw.Draw(sheet)
y = 10
for label, tile in tiles:
    d.text((20, y), label, font=ImageFont.truetype(os.path.join(REPO, "src", "bin", "fonts", "LiberationSans-Regular.ttf"), 14), fill=(235, 235, 235, 255))
    sheet.alpha_composite(tile, (20, y + 18))
    y += tile.height + 36
sheet.convert("RGB").save(os.path.join(os.path.dirname(__file__), "exp_hunt_bar_preview.png"))
print("wrote the preview")
