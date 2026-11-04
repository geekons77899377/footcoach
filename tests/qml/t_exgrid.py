# Bibliothèque d'exercices : grille de cartes (bureau/tablette), liste (téléphone), ouverture d'une fiche.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF
import shutil

def F(h, name, all_=False):
    todo = [h.win.contentItem()]; out = []
    while todo:
        it = todo.pop()
        if it.objectName() == name: out.append(it)
        todo.extend(it.childItems())
    return out if all_ else (out[0] if out else None)
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".work", "out")
def shot(h, tag):
    h.shot("x"); shutil.copy(os.path.join(OUT, "x.png"), os.path.join(os.environ.get("HOME_SHOTS", OUT), tag + ".png"))
def make(w, hh):
    B2.mode = "now"; b = B2(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", -1)); h.spin(600)
    return h
def page(h): return F(h, "exerciseGrid").parentItem().parent()
def visible_cards(h):
    cs = [c for c in F(h, "exerciseCard", True) if c.isVisible()]
    return sorted(cs, key=lambda c: (round(c.mapToScene(QPointF(0, 0)).y()), c.mapToScene(QPointF(0, 0)).x()))
def click_item(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); h.click(p.x(), p.y())

for name, w, hh, cols_min in [("bureau", 1280, 800, 3), ("tablette-paysage", 1024, 768, 2), ("tablette-portrait", 800, 1280, 2)]:
    h = make(w, hh); pg = page(h); g = F(h, "exerciseGrid")
    check(g.isVisible() and pg.property("gridMode") is True, f"{name} : grille affichée")
    cards = visible_cards(h)
    check(len(cards) == 4, f"{name} : 4 cartes (une par exercice) -> {len(cards)}")
    xs = sorted(set(round(c.mapToScene(QPointF(0, 0)).x()) for c in cards))
    check(len(xs) >= cols_min or len(cards) < cols_min, f"{name} : au moins {cols_min} colonnes ({len(xs)})")
    check(all(c.mapToScene(QPointF(c.width(), 0)).x() <= w - 8 for c in cards), f"{name} : cartes dans la fenêtre")
    shot(h, f"ex_{name}")
    click_item(h, cards[1]); h.spin(500)
    check(pg.property("editing") is True, f"{name} : clic sur une carte ouvre la fiche")
    check(not F(h, "exerciseGrid").isVisible(), f"{name} : grille masquée pendant la fiche")
    if pg.width() >= 900:
        check(pg.property("split") is True and F(h, "exerciseGrid").parentItem().width() < w, f"{name} : liste + fiche côte à côte")
    shot(h, f"ex_{name}_fiche")
    check(pg.property("currentId") == 11, f"{name} : bon exercice ouvert (id {pg.property('currentId')})")
    ok = QMetaObject.invokeMethod(pg, "goBack"); h.spin(300)
    check(pg.property("editing") is False and F(h, "exerciseGrid").isVisible(), f"{name} : retour -> grille")

for name, w, hh in [("telephone", 412, 892), ("telephone-paysage", 844, 390)]:
    h = make(w, hh); g = F(h, "exerciseGrid")
    check(not g.isVisible() and len(visible_cards(h)) == 0, f"{name} : pas de grille (liste inchangée)")
    if name == "telephone": shot(h, "ex_telephone")

# Filtre : la recherche agit aussi sur la grille.
h = make(1280, 800); pg = page(h)
pg.setProperty("query", "rondo"); h.spin(300)
check(len(visible_cards(h)) == 1, f"filtre « rondo » -> 1 carte ({len(visible_cards(h))})")
pg.setProperty("query", "zzz"); h.spin(300)
check(len(visible_cards(h)) == 0, "filtre sans résultat -> 0 carte")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
