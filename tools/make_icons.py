#!/usr/bin/env python3
"""Génère les icônes FootCoach à partir de store/icon.svg (source unique, 1024 × 1024, deux groupes : « fond » et
« premier-plan »).

Produit : icône 512 × 512 du Play Store, icônes Android adaptatives (fond + premier plan) et classiques, icône Windows.
Usage : python3 tools/make_icons.py   (nécessite Playwright + Chromium, et Pillow pour le .ico)
Les fichiers générés sont versionnés ; ce script ne sert qu'à les refaire.
"""
import pathlib, re
from playwright.sync_api import sync_playwright
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent
RES = ROOT / "android" / "res"
SRC = (ROOT / "store" / "icon.svg").read_text()

# Zone sûre d'une icône adaptative : disque de 66 dp au centre d'un canevas de 108 dp. Le contenu du premier plan
# (but, joueur, ballon) tient dans un cercle d'environ 860 unités sur 1024 : une unité vaut donc 66/860 dp.
UNIT_DP = 66 / 860
ADAPTIVE_VIEW = 108 / UNIT_DP            # largeur du canevas adaptatif, en unités du dessin


def with_view(svg, x, y, w, hide=None):
    """Même dessin, avec un autre cadrage ; `hide` : id d'un groupe à masquer."""
    out = re.sub(r'viewBox="[^"]*"', f'viewBox="{x:.2f} {y:.2f} {w:.2f} {w:.2f}"', svg, count=1)
    if hide:
        out = out.replace(f'<g id="{hide}"', f'<g id="{hide}" display="none"', 1)
    return out


def main():
    centre = 512
    adaptive_x = centre - ADAPTIVE_VIEW / 2
    dens = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}
    with sync_playwright() as p:
        b = p.chromium.launch()
        page = b.new_page()

        def shot(markup, px, dest, transparent=False):
            page.set_viewport_size({"width": px, "height": px})
            page.set_content(f'<html><body style="margin:0;background:transparent">{markup.replace("width=\"1024\" height=\"1024\"", f"width=\"{px}\" height=\"{px}\"", 1)}</body></html>')
            dest.parent.mkdir(parents=True, exist_ok=True)
            page.screenshot(path=str(dest), omit_background=transparent, clip={"x": 0, "y": 0, "width": px, "height": px})

        # 1) Play Store : 512 × 512, opaque.
        shot(SRC, 512, ROOT / "store" / "icon-512.png")
        for name, k in dens.items():
            px = round(108 * k)
            # 2) Adaptative : fond (pelouse) et premier plan (transparent) cadrés pour respecter la zone sûre.
            shot(with_view(SRC, adaptive_x, adaptive_x, ADAPTIVE_VIEW, hide="premier-plan"), px, RES / f"drawable-{name}" / "icon_background.png")
            shot(with_view(SRC, adaptive_x, adaptive_x, ADAPTIVE_VIEW, hide="fond"), px, RES / f"drawable-{name}" / "icon_foreground.png", True)
            # 3) Classique (avant Android 8) : carré arrondi, dessin complet.
            px48 = round(48 * k)
            rounded = SRC.replace("<g id=\"fond\"", f'<clipPath id="rr"><rect width="1024" height="1024" rx="{1024*0.22:.0f}"/></clipPath><g clip-path="url(#rr)"><g id="fond"', 1).replace("</svg>", "</g></svg>")
            shot(rounded, px48, RES / f"drawable-{name}" / "icon.png", True)
        # 4) Windows : .ico multi-tailles, dessiné depuis le dessin complet.
        sizes = [16, 24, 32, 48, 64, 128, 256]
        tmp = ROOT / "windows" / "_icon-256.png"
        (ROOT / "windows").mkdir(exist_ok=True)
        shot(rounded, 256, tmp, True)
        b.close()
    Image.open(tmp).convert("RGBA").save(ROOT / "windows" / "footcoach.ico", sizes=[(s, s) for s in sizes])
    tmp.unlink()

    (RES / "drawable-anydpi-v26").mkdir(parents=True, exist_ok=True)
    (RES / "drawable-anydpi-v26" / "icon.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@drawable/icon_background"/>\n'
        '    <foreground android:drawable="@drawable/icon_foreground"/>\n'
        '</adaptive-icon>\n')
    (RES / "values").mkdir(parents=True, exist_ok=True)
    (RES / "values" / "footcoach_icon.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n'
        '    <color name="footcoach_icon_background">#14531a</color>\n</resources>\n')


if __name__ == "__main__":
    main()
