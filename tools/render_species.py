# /// script
# requires-python = ">=3.10"
# dependencies = ["pillow"]
# ///
"""
Render every ASCII species in src/buddies/ to PNG so they can be compared
side by side without flashing the device.

Writes docs/species/<name>.png (first pose of each of the 7 states) and
docs/species/_all.png (idle pose of every species on one sheet). Species
whose art is wider than the screen at 2x are marked "clips".

Usage:
  uv run tools/render_species.py
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont
from species import FIT_COLS, PROJECT, STATES, load_all

OUT = PROJECT / "docs" / "species"
FONTS = ["/System/Library/Fonts/Menlo.ttc",
         "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"]
SIZE, PAD, DIM, WARN = 22, 14, (132, 130, 132), (250, 68, 0)


def font(size):
    for f in FONTS:
        if Path(f).exists():
            return ImageFont.truetype(f, size)
    sys.exit("no monospace font found; add one to FONTS")


def draw_pose(d, lines, x, y, color, fnt, ch):
    for i, line in enumerate(lines):
        d.text((x, y + i * ch), line, font=fnt, fill=color)


def label(d, x, y, sp, small):
    d.text((x, y), sp.name, font=small, fill="white")
    if sp.ink_width > FIT_COLS:
        d.text((x + small.getlength(sp.name) + 10, y),
               f"clips ({sp.ink_width} wide)", font=small, fill=WARN)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    fnt, small = font(SIZE), font(13)
    cw, ch = fnt.getlength("M"), SIZE + 2
    species = load_all()
    cols = max(len(line) for sp in species for st in STATES for line in sp.first(st))
    pw, ph = int(cols * cw) + PAD * 2, 5 * ch + PAD * 2 + 20

    for sp in species:
        img = Image.new("RGB", (pw * len(STATES), ph + 30), "black")
        d = ImageDraw.Draw(img)
        label(d, PAD, 8, sp, small)
        for i, st in enumerate(STATES):
            draw_pose(d, sp.first(st), i * pw + PAD, 30 + PAD, sp.rgb, fnt, ch)
            d.text((i * pw + PAD, 30 + ph - 22), st, font=small, fill=DIM)
        img.save(OUT / f"{sp.name}.png")

    per_row = 6
    n_rows = -(-len(species) // per_row)
    sheet = Image.new("RGB", (pw * per_row, ph * n_rows), "black")
    d = ImageDraw.Draw(sheet)
    for i, sp in enumerate(species):
        x, y = (i % per_row) * pw, (i // per_row) * ph
        draw_pose(d, sp.first("idle"), x + PAD, y + PAD, sp.rgb, fnt, ch)
        label(d, x + PAD, y + ph - 22, sp, small)
    sheet.save(OUT / "_all.png")
    print(f"wrote {len(species)} species + _all.png to {OUT.relative_to(PROJECT)}/")


if __name__ == "__main__":
    main()
