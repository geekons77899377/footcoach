"""Lot 1 — navigation mobile : six entrées (Accueil, Séances, Exercices, Calendrier, Effectif, Suivi), sans « Plus ».
Vérifie à 360 px et 412 px : entrées, dimensions tactiles, absence de chevauchement, libellés sans troncature,
navigation au toucher, accès explicites Tactique / Données / Paramètres, barre masquée en édition plein écran.
Usage : python3 tests/qml/t_nav.py   (exécuté aussi par run_all.py)"""
import os, sys
HERE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE_DIR)
os.chdir(HERE_DIR)
exec(open(os.path.join(HERE_DIR, 't_live.py'), encoding='utf-8').read().split("errs=[]")[0])
from PySide6.QtCore import Qt, Q_ARG, QMetaObject, QObject, QPointF
import json

errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

EXPECTED = [("home", "Accueil"), ("sessions", "Séances"), ("exercises", "Exercices"),
            ("calendar", "Calendrier"), ("squad", "Effectif"), ("attendance", "Suivi")]
SHOTS = os.environ.get("NAV_SHOTS", "1") == "1"
from harness import OUT

def prop(o, p): return o.property(p)
def named(root, prefix, acc=None):
    """Parcourt l'arbre visuel (les délégués d'un Repeater ne sont pas accessibles par findChild)."""
    acc = {} if acc is None else acc
    for c in root.childItems():
        n = c.objectName()
        if n and n.startswith(prefix): acc[n] = c
        named(c, prefix, acc)
    return acc
def flickables(root, acc=None):
    acc = [] if acc is None else acc
    for c in root.childItems():
        if c.metaObject().className() == "QQuickFlickable": acc.append(c)
        flickables(c, acc)
    return acc
def bring_into_view(h, item):
    """Fait défiler l'accueil jusqu'à rendre l'élément visible, puis renvoie son centre (coordonnées de fenêtre)."""
    for fl in flickables(h.win.contentItem()):
        p = item.mapToItem(fl, QPointF(0, 0))
        if 0 <= p.y() <= prop(fl, "contentHeight") + 400 and prop(fl, "contentHeight") > prop(fl, "height"):
            fl.setProperty("contentY", max(0, prop(fl, "contentY") + p.y() - 120)); h.spin(300)
            break
    return centre(item)
def tab_of(h, i):
    return named(h.find("bottomBar"), "navTab_").get(f"navTab_{i}")
def label_of(h, i):
    return named(h.find("bottomBar"), "navLabel_").get(f"navLabel_{i}")
def centre(o):
    p = o.mapToScene(QPointF(prop(o, "width") / 2, prop(o, "height") / 2))
    return p.x(), p.y()

for (w, hgt) in [(360, 740), (412, 892)]:
    tag = f"[{w}px]"
    h, b = run(w, hgt)
    win = h.win
    tabs = [tab_of(h, i) for i, _ in EXPECTED]
    if not all(t is not None for t in tabs):
        print("     onglets trouvés :", sorted(named(h.find("bottomBar"), "navTab_")))
    check(all(t is not None for t in tabs), f"{tag} six entrées présentes")
    check(tab_of(h, "more") is None, f"{tag} aucune entrée « Plus »")
    texts = [prop(label_of(h, i), "text") for i, _ in EXPECTED]
    check(texts == ["Accueil", "Séances", "Exercices", "Calendrier", "Effectif", "Suivi"], f"{tag} libellés : {texts}")

    # Dimensions tactiles et chevauchement
    xs = []
    for t, (i, _) in zip(tabs, EXPECTED):
        tw, th = prop(t, "width"), prop(t, "height")
        check(tw >= 48 and th >= 48, f"{tag} cible tactile {i} : {tw:.0f}×{th:.0f} px (min 48)")
        xs.append((prop(t, "x"), tw))
    ok_overlap = all(xs[k][0] + xs[k][1] <= xs[k + 1][0] + 0.5 for k in range(len(xs) - 1))
    check(ok_overlap, f"{tag} aucun chevauchement entre entrées")
    sizes = {prop(label_of(h, i), "font").pixelSize() for i, _ in EXPECTED}
    check(sizes == {10}, f"{tag} taille des libellés uniforme : {sorted(sizes)} px")
    for (i, _) in EXPECTED:
        lab = label_of(h, i)
        pw, pw_box = prop(lab, "paintedWidth"), prop(lab, "width")
        ph, bh = prop(lab, "paintedHeight"), prop(lab, "height")
        check(pw <= pw_box + 0.5, f"{tag} libellé « {prop(lab,'text')} » tient dans son entrée ({pw:.0f}/{pw_box:.0f} px, {prop(lab,'font').pixelSize()} px)")

    # Navigation au toucher : chaque entrée ouvre sa section
    h.resize(w, hgt)
    for (i, name), t in zip(EXPECTED, tabs):
        x, y = centre(t)
        h.click(x, y)
        check(prop(win, "section") == i, f"{tag} toucher « {name} » -> section {i}")

    # Accès explicites depuis l'accueil
    h.click(*centre(tabs[0]))
    dataB, setB = h.find("dataButton"), h.find("settingsButton")
    if dataB is None: dataB = named(h.win, "dataButton").get("dataButton")
    check(dataB is not None and setB is not None, f"{tag} boutons Données et Paramètres présents")
    if dataB is not None:
        x, y = bring_into_view(h, dataB)
        if 0 <= y <= hgt - 72:
            h.click(x, y); check(prop(win, "section") == "data", f"{tag} accueil -> Données")
        else:
            check(False, f"{tag} bouton Données hors écran (y={y:.0f}) : défilement requis")
    if setB is not None:
        h.click(*centre(tabs[0]))
        x, y = bring_into_view(h, setB)
        if 0 <= y <= hgt - 72:
            h.click(x, y); check(prop(win, "section") == "settings", f"{tag} accueil -> Paramètres")
        else:
            check(False, f"{tag} bouton Paramètres hors écran (y={y:.0f}) : défilement requis")
    h.click(*centre(tabs[0]))
    # Tactique : carte « Ouvrir » de l'accueil
    open_btn = [o for o in h.win.findChildren(QObject) if prop(o, "text") == "Ouvrir" and prop(o, "iconName") == "board"]
    if open_btn:
        h.click(*centre(tabs[0]))
        x, y = bring_into_view(h, open_btn[0])
        if 0 <= y <= hgt - 72:
            h.click(x, y); check(prop(win, "section") == "tactics", f"{tag} accueil -> Tactique")
        else:
            check(False, f"{tag} bouton Tactique hors écran (y={y:.0f})")
    else:
        check(False, f"{tag} bouton Tactique introuvable")

    # Barre masquée pendant l'édition plein écran du terrain (comportement existant préservé)
    bid = b.add("Pressing", json.dumps({"version": 1, "pitch": "full", "items": [], "arrows": []}))
    QMetaObject.invokeMethod(win, "go", Qt.DirectConnection, Q_ARG("QVariant", "tactics"), Q_ARG("QVariant", bid))
    h.spin(400)
    check(prop(h.find("bottomBar"), "visible") is False, f"{tag} barre masquée en édition du terrain (plein écran)")
    QMetaObject.invokeMethod(win, "handleBack", Qt.DirectConnection)
    h.spin(300)

    if SHOTS:
        for (i, name) in EXPECTED:
            QMetaObject.invokeMethod(win, "go", Qt.DirectConnection, Q_ARG("QVariant", i), Q_ARG("QVariant", -1))
            h.shot(f"nav_{w}_{i}")
        QMetaObject.invokeMethod(win, "go", Qt.DirectConnection, Q_ARG("QVariant", "home"), Q_ARG("QVariant", -1))
        h.spin(200)

print("\nTOUT PASSE" if not errs else f"\nECHECS : {errs}")
sys.exit(1 if errs else 0)
