#!/usr/bin/env python3
"""Génère l'icône FootCoach (ballon rond vert) : SVG source, icônes Android et icône 512 × 512 du Play Store.

Usage : python3 tools/make_icons.py   (nécessite Playwright + Chromium pour la mise en pixels)
Les fichiers générés sont versionnés ; ce script ne sert qu'à les refaire.
"""
import math, os, pathlib
from playwright.sync_api import sync_playwright

ROOT = pathlib.Path(__file__).resolve().parent.parent
RES = ROOT / "android" / "res"
BG = "#0f172a"          # fond de l'application (bleu nuit)
GREEN_LIGHT, GREEN_DARK = "#4ade80", "#15803d"
PATTERN = "#0b2e1a"


def pent(cx, cy, r, rot):
    return [(cx + r * math.cos(math.radians(rot + 72 * i)), cy + r * math.sin(math.radians(rot + 72 * i))) for i in range(5)]


def pts(p):
    return " ".join(f"{x:.2f},{y:.2f}" for x, y in p)


def ball(cx, cy, R, uid):
    """Ballon centré en (cx, cy), de rayon R : pentagone central, rayons et pentagones de bord coupés par le cercle."""
    s = []
    s.append(f'<defs><radialGradient id="g{uid}" cx="38%" cy="32%" r="80%"><stop offset="0" stop-color="{GREEN_LIGHT}"/>'
             f'<stop offset="1" stop-color="{GREEN_DARK}"/></radialGradient>'
             f'<clipPath id="c{uid}"><circle cx="{cx}" cy="{cy}" r="{R}"/></clipPath></defs>')
    s.append(f'<circle cx="{cx}" cy="{cy}" r="{R}" fill="url(#g{uid})"/>')
    s.append(f'<g clip-path="url(#c{uid})" fill="{PATTERN}" stroke="{PATTERN}" stroke-linejoin="round" stroke-linecap="round">')
    rc = R * 0.34
    center = pent(cx, cy, rc, -90)
    s.append(f'<polygon points="{pts(center)}" stroke-width="{R*0.02:.2f}"/>')
    for i, (vx, vy) in enumerate(center):
        a = math.radians(-90 + 72 * i)
        ox, oy = cx + R * 0.66 * math.cos(a), cy + R * 0.66 * math.sin(a)
        s.append(f'<line x1="{vx:.2f}" y1="{vy:.2f}" x2="{ox:.2f}" y2="{oy:.2f}" stroke-width="{R*0.045:.2f}" fill="none"/>')
        # pentagone de bord, pointe tournée vers le centre
        px, py = cx + R * 1.0 * math.cos(a), cy + R * 1.0 * math.sin(a)
        s.append(f'<polygon points="{pts(pent(px, py, R*0.36, -90 + 72*i + 180))}" stroke-width="{R*0.02:.2f}"/>')
        # couture entre deux pentagones de bord
        b = math.radians(-90 + 72 * i + 36)
        qx, qy = cx + R * 0.70 * math.cos(b), cy + R * 0.70 * math.sin(b)
        ex, ey = cx + R * 1.0 * math.cos(b), cy + R * 1.0 * math.sin(b)
        s.append(f'<line x1="{qx:.2f}" y1="{qy:.2f}" x2="{ex:.2f}" y2="{ey:.2f}" stroke-width="{R*0.04:.2f}" fill="none"/>')
    s.append("</g>")
    s.append(f'<circle cx="{cx}" cy="{cy}" r="{R - R*0.012:.2f}" fill="none" stroke="#052e16" stroke-opacity=".55" stroke-width="{R*0.024:.2f}"/>')
    return "".join(s)


def svg(size, content, background=None):
    bg = f'<rect width="{size}" height="{size}" fill="{background}"/>' if background else ""
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" viewBox="0 0 {size} {size}">{bg}{content}</svg>'


def main():
    out = {}
    # 1) Source : icône pleine (512), fond bleu nuit, ballon à 72 % de la largeur.
    full = svg(512, ball(256, 256, 184, "f"), BG)
    (ROOT / "store").mkdir(exist_ok=True)
    (ROOT / "store" / "icon.svg").write_text(full)
    # 2) Premier plan adaptatif (108 dp) : zone sûre = disque de 66 dp au centre, donc rayon du ballon 30 dp.
    fg = lambda px: svg(px, ball(px / 2, px / 2, px * 30 / 108, "a"))
    # 3) Icône classique (48 dp) : fond arrondi.
    def legacy(px):
        r = px * 0.22
        body = f'<rect width="{px}" height="{px}" rx="{r:.1f}" fill="{BG}"/>' + ball(px / 2, px / 2, px * 0.36, "l")
        return svg(px, body)
    dens = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}
    with sync_playwright() as p:
        b = p.chromium.launch()
        page = b.new_page()
        def shot(markup, px, dest):
            page.set_viewport_size({"width": px, "height": px})
            page.set_content(f'<html><body style="margin:0;background:transparent">{markup}</body></html>')
            dest.parent.mkdir(parents=True, exist_ok=True)
            page.screenshot(path=str(dest), omit_background=True, clip={"x": 0, "y": 0, "width": px, "height": px})
        shot(full, 512, ROOT / "store" / "icon-512.png")
        for name, k in dens.items():
            shot(fg(round(108 * k)), round(108 * k), RES / f"drawable-{name}" / "icon_foreground.png")
            shot(legacy(round(48 * k)), round(48 * k), RES / f"drawable-{name}" / "icon.png")
        b.close()
    (RES / "drawable-anydpi-v26").mkdir(parents=True, exist_ok=True)
    (RES / "drawable-anydpi-v26" / "icon.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
        '    <background android:drawable="@color/footcoach_icon_background"/>\n'
        '    <foreground android:drawable="@drawable/icon_foreground"/>\n'
        '</adaptive-icon>\n')
    (RES / "values").mkdir(parents=True, exist_ok=True)
    (RES / "values" / "footcoach_icon.xml").write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n<resources>\n'
        f'    <color name="footcoach_icon_background">{BG}</color>\n</resources>\n')


if __name__ == "__main__":
    main()
