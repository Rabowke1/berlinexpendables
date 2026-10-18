#!/usr/bin/env python3
"""pixelize – macht aus KI-Output (AutoSprite, ComfyUI) echte Pixel-Art im Berlin-Nacht-Stil.

Schritte: Nearest-Neighbor-Downscale auf das Zielraster, Quantisierung auf die Projektpalette
(optional mit Ordered-Dither), Entfernen von Einzelpixel-Rauschen, 1-px-Outline, optional
Kachelbarkeits-Check und Normalmap aus geschätzter Höhe (für beleuchtete Sprites in UE).

Beispiele:
  python Tools/pixelize/pixelize.py in.png out.png --width 64 --palette Prototype/Web/public/assets/palette.json --outline
  python Tools/pixelize/pixelize.py bg.png bg_px.png --width 960 --dither --tile-check
  python Tools/pixelize/pixelize.py sheet.png sheet_px.png --grid 64 --normals sheet_n.png
"""
import argparse
import json
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    sys.exit("Pillow fehlt: pip install pillow")

BAYER4 = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def load_palette(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    cols = data.values() if isinstance(data, dict) else data
    out = []
    for h in cols:
        h = h.lstrip("#")
        out.append(tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)))
    return out


def nearest(color, palette):
    r, g, b = color
    best, bd = palette[0], 1e18
    for p in palette:
        # gewichtete Distanz (Auge ist grünempfindlicher)
        d = 0.3 * (r - p[0]) ** 2 + 0.59 * (g - p[1]) ** 2 + 0.11 * (b - p[2]) ** 2
        if d < bd:
            bd, best = d, p
    return best


def downscale(img, width=None, height=None, grid=None):
    if grid:
        return img  # Sheets mit festem Frame-Raster werden nicht skaliert, nur quantisiert
    if width and not height:
        height = round(img.height * width / img.width)
    if height and not width:
        width = round(img.width * height / img.height)
    if width and height and (width != img.width or height != img.height):
        # erst leicht weichzeichnen (Box), dann Nearest – verhindert Flimmer-Pixel aus Hi-Res-Rauschen
        box = img.resize((width, height), Image.BOX)
        return box
    return img


def quantize(img, palette, dither=False, alpha_cut=128):
    img = img.convert("RGBA")
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = px[x, y]
            if a < alpha_cut:
                px[x, y] = (0, 0, 0, 0)
                continue
            if dither:
                t = (BAYER4[y % 4][x % 4] / 16.0 - 0.5) * 24
                r, g, b = (max(0, min(255, int(c + t))) for c in (r, g, b))
            nr, ng, nb = nearest((r, g, b), palette)
            px[x, y] = (nr, ng, nb, 255)
    return img


def despeckle(img):
    """Einzelne Pixel, deren 4 Nachbarn alle eine andere, untereinander gleiche Farbe haben, angleichen."""
    px = img.load()
    out = img.copy(); opx = out.load()
    for y in range(1, img.height - 1):
        for x in range(1, img.width - 1):
            c = px[x, y]
            n = [px[x - 1, y], px[x + 1, y], px[x, y - 1], px[x, y + 1]]
            if all(v == n[0] for v in n) and n[0] != c:
                opx[x, y] = n[0]
    return out


def outline(img, color=(11, 10, 18)):
    px = img.load()
    out = img.copy(); opx = out.load()
    for y in range(img.height):
        for x in range(img.width):
            if px[x, y][3] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < img.width and 0 <= yy < img.height and px[xx, yy][3] != 0:
                    opx[x, y] = (*color, 255)
                    break
    return out


def tile_check(img):
    """Meldet die mittlere Farbdifferenz an den Nahtkanten (links/rechts). < 20 ist gut kachelbar."""
    px = img.convert("RGB").load()
    diff = 0
    for y in range(img.height):
        a, b = px[0, y], px[img.width - 1, y]
        diff += sum(abs(a[i] - b[i]) for i in range(3)) / 3
    return diff / max(1, img.height)


def normal_map(img, strength=2.0):
    """Höhe aus Helligkeit schätzen (hell = vorne), Sobel-Normalen, Tangentraum (X rechts, Y oben, Z raus)."""
    gray = img.convert("L"); gp = gray.load()
    alpha = img.getchannel("A").load()
    w, h = img.size
    out = Image.new("RGB", (w, h), (128, 128, 255)); op = out.load()

    def hgt(x, y):
        x = min(max(x, 0), w - 1); y = min(max(y, 0), h - 1)
        return gp[x, y] / 255.0 if alpha[x, y] else 0.0

    for y in range(h):
        for x in range(w):
            if not alpha[x, y]:
                continue
            dx = (hgt(x + 1, y - 1) + 2 * hgt(x + 1, y) + hgt(x + 1, y + 1)) - (hgt(x - 1, y - 1) + 2 * hgt(x - 1, y) + hgt(x - 1, y + 1))
            dy = (hgt(x - 1, y + 1) + 2 * hgt(x, y + 1) + hgt(x + 1, y + 1)) - (hgt(x - 1, y - 1) + 2 * hgt(x, y - 1) + hgt(x + 1, y - 1))
            nx, ny, nz = -dx * strength, dy * strength, 1.0
            l = (nx * nx + ny * ny + nz * nz) ** 0.5
            op[x, y] = (int((nx / l * 0.5 + 0.5) * 255), int((ny / l * 0.5 + 0.5) * 255), int((nz / l * 0.5 + 0.5) * 255))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src"); ap.add_argument("dst")
    ap.add_argument("--width", type=int); ap.add_argument("--height", type=int)
    ap.add_argument("--grid", type=int, help="Sheet mit festem Frame-Raster: nicht skalieren, nur quantisieren")
    ap.add_argument("--palette", default="Prototype/Web/public/assets/palette.json")
    ap.add_argument("--dither", action="store_true"); ap.add_argument("--no-despeckle", action="store_true")
    ap.add_argument("--outline", action="store_true", help="1-px-ink-Outline hinzufügen (Figuren, Props)")
    ap.add_argument("--alpha-cut", type=int, default=128)
    ap.add_argument("--tile-check", action="store_true")
    ap.add_argument("--normals", help="Normalmap zusätzlich nach <pfad> schreiben")
    ap.add_argument("--scale-out", type=int, default=1, help="Ausgabe zusätzlich ganzzahlig hochskalieren (Vorschau)")
    a = ap.parse_args()

    palette = load_palette(a.palette)
    img = Image.open(a.src).convert("RGBA")
    img = downscale(img, a.width, a.height, a.grid)
    img = quantize(img, palette, a.dither, a.alpha_cut)
    if not a.no_despeckle:
        img = despeckle(img)
    if a.outline:
        img = outline(img)
    if a.tile_check:
        print(f"Nahtdifferenz links/rechts: {tile_check(img):.1f} (unter 20 = gut kachelbar)")
    if a.normals:
        normal_map(img).save(a.normals)
        print(f"Normalmap: {a.normals}")
    if a.scale_out > 1:
        img = img.resize((img.width * a.scale_out, img.height * a.scale_out), Image.NEAREST)
    img.save(a.dst)
    used = len({px for px in img.convert('RGBA').getdata() if px[3]})
    print(f"{a.dst}: {img.width}x{img.height}, {used} Farben")


if __name__ == "__main__":
    main()
