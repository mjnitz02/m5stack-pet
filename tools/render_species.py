# /// script
# requires-python = ">=3.10"
# dependencies = ["pillow"]
# ///
"""
Render every ASCII species in src/buddies/ to PNG so they can be compared
side by side without flashing the device.

The pictures use the stick's own font and its layout rules, pixel for pixel:
at home-screen size every row is trimmed and centred by itself, at activity
size the rows keep their padding. Particles, hats and dress-up colours are
not drawn.

Writes docs/species/<name>.png (first pose of each of the 7 states, at both
sizes), docs/species/_all.png (idle pose of every species, home size) and
docs/home.png (a mock of the pet screen, laid out as drawBottom() in
src/main.cpp does it).

Usage:
  uv run tools/render_species.py
"""
from glcdfont import FIRST, GLYPHS
from PIL import Image, ImageDraw
from species import PROJECT, STATES, load_all

OUT = PROJECT / "docs" / "species"
SCREEN_W, CHAR_W, CHAR_H = 135, 6, 8   # buddy.cpp: BUDDY_CANVAS_W, BUDDY_CHAR_W, BUDDY_CHAR_H
ZOOM, GAP, DIM, WHITE = 2, 6, (132, 130, 132), (255, 255, 255)


def text(img, s, x, y, scale, color):
    for ch in s:
        i = (ord(ch) - FIRST) * 5
        for col in range(5):
            bits = GLYPHS[i + col] if 0 <= i < len(GLYPHS) else 0
            for row in range(8):
                if bits >> row & 1:
                    img.paste(color, (x + col * scale, y + row * scale,
                                      x + (col + 1) * scale, y + (row + 1) * scale))
        x += CHAR_W * scale


def pose(lines, scale, color):
    """One pose as buddyPrintLine() places it, on a screen-wide black strip."""
    img = Image.new("RGB", (SCREEN_W, (len(lines) * CHAR_H + 4) * scale), "black")
    for i, line in enumerate(lines):
        if scale > 1:
            line = line.strip()
        x = SCREEN_W // 2 - len(line) * CHAR_W * scale // 2
        text(img, line, x, (2 + i * CHAR_H) * scale, scale, color)
    return img


def save(img, path, zoom=ZOOM):
    img.resize((img.width * zoom, img.height * zoom), Image.NEAREST).save(path)


def home(sp):
    """The pet screen: battery trim, pet, apples, hearts and the three icons."""
    red, green, yel, aqua, rose, amber = (255, 0, 0), (0, 255, 0), (255, 255, 0), (0, 255, 255), (255, 109, 181), (255, 166, 0)
    img = Image.new("RGB", (SCREEN_W, 240), "black")
    d = ImageDraw.Draw(img)
    disc = lambda x, y, r, c, fill=True: d.ellipse((x - r, y - r, x + r, y + r), fill=c if fill else None, outline=c)
    for i in range(5):
        d.rectangle((41 + i * 11, 1, 49 + i * 11, 2), fill=DIM)
    img.paste(pose(sp.first("idle"), 2, sp.rgb), (0, 42))
    for i in range(4):                                   # three apples, two hearts
        x = 34 + i * 22
        disc(x, 177, 4, red if i < 3 else DIM, i < 3)
        if i < 2:
            disc(x - 2, 190, 2, red)
            disc(x + 2, 190, 2, red)
            d.polygon([(x - 4, 191), (x + 4, 191), (x, 195)], fill=red)
        else:
            disc(x - 2, 190, 2, DIM, False)
            disc(x + 2, 190, 2, DIM, False)
            d.line([(x - 4, 191), (x, 195), (x + 4, 191)], fill=DIM)
    d.line([(0, 201), (SCREEN_W, 201)], fill=DIM)
    x, y = 22, 224                                       # feed
    disc(x, y, 9, red)
    d.line([(x, y - 9), (x + 1, y - 14)], fill=DIM)
    d.polygon([(x + 1, y - 10), (x + 8, y - 12), (x + 4, y - 8)], fill=green)
    d.rounded_rectangle((x - 20, 204, x + 19, 238), 6, outline=WHITE, width=2)
    x, y = 67, 222                                       # dance party
    d.polygon([(x - 8, y + 9), (x + 8, y + 9), (x, y - 8)], fill=aqua)
    d.rectangle((x - 4, y, x + 4, y + 1), fill=rose)
    disc(x, y - 8, 2, yel)
    for dx, dy, c in [(-14, -6, rose), (11, -3, yel), (-13, 3, green), (13, 6, amber), (7, -11, green), (-9, -12, amber)]:
        d.rectangle((x + dx, y + dy, x + dx + 2, y + dy + 2), fill=c)
    x = 112                                              # bed
    disc(x - 2, y, 9, yel)
    disc(x + 2, y - 3, 8, (0, 0, 0))
    text(img, "z", x + 5, y - 2, 1, WHITE)
    return img


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    species = load_all()
    for old in OUT.glob("*.png"):
        old.unlink()
    home_h, small_h, label_h = 44 * 2, 44, CHAR_H + 4

    for sp in species:
        sheet = Image.new("RGB", ((SCREEN_W + GAP) * len(STATES) - GAP,
                                  label_h + home_h + GAP + small_h + label_h), (24, 24, 24))
        text(sheet, sp.name, 2, 2, 1, WHITE)
        for i, st in enumerate(STATES):
            x = i * (SCREEN_W + GAP)
            sheet.paste(pose(sp.first(st), 2, sp.rgb), (x, label_h))
            sheet.paste(pose(sp.first(st), 1, sp.rgb), (x, label_h + home_h + GAP))
            text(sheet, st, x + 2, label_h + home_h + GAP + small_h + 2, 1, DIM)
        save(sheet, OUT / f"{sp.name}.png")

    sheet = Image.new("RGB", ((SCREEN_W + GAP) * len(species) - GAP, home_h + label_h), (24, 24, 24))
    for i, sp in enumerate(species):
        x = i * (SCREEN_W + GAP)
        sheet.paste(pose(sp.first("idle"), 2, sp.rgb), (x, 0))
        text(sheet, sp.name, x + 2, home_h + 2, 1, WHITE)
    save(sheet, OUT / "_all.png")
    save(home(next(sp for sp in species if sp.name == "octopus")), OUT.parent / "home.png", 3)
    print(f"wrote {len(species)} species + _all.png to {OUT.relative_to(PROJECT)}/")


if __name__ == "__main__":
    main()
