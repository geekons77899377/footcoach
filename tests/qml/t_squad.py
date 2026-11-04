# Effectif : grille de cartes (tablette), liste + vue d'ensemble (bureau), liste (téléphone), états vides, ouverture de fiche.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF
import shutil

PL = [(1,"Lucas","Silva",9,"FWD",22,True),(2,"Marco","Reus",11,"MID",30,True),(3,"André","Gomes",4,"DEF",25,True),
      (4,"Yann","Portier",1,"GK",28,True),(5,"Tom","Ancien",7,"MID",33,False)]
def pm(t):
    i,f,l,n,pos,age,act = t
    return {"id":i,"teamId":1,"firstName":f,"lastName":l,"fullName":f+" "+l,"birthDate":"","age":age,"jerseyNumber":n,"position":pos,
            "preferredFoot":"","phone":"","email":"","notes":"","active":act}
class B4(B2):
    empty = False
    noteam = False
    @Slot(result="QVariantList")
    def teams(self): return [] if self.noteam else [{"id":1,"name":"Senior A","category":"Seniors","playerCount":4}]
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive):
        if self.empty: return []
        return [pm(t) for t in PL if t[6] or inactive]
    @Slot(int, str, str, result="QVariantMap")
    def attendanceStats(self, team, a, b):
        return {"sessions":[{"id":1,"date":"2026-10-30"}],
                "players":[{"id":1,"name":"Lucas Silva","rate":0.95},{"id":2,"name":"Marco Reus","rate":0.5},{"id":3,"name":"André Gomes","rate":0.8}]}

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
def make(w, hh, empty=False, noteam=False):
    B4.empty = empty; B4.noteam = noteam
    b = B4(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "squad"), Q_ARG("QVariant", -1)); h.spin(700)
    return h
def page(h): return F(h, "playerGrid").parentItem().parent()
def cards(h):
    cs = [c for c in F(h, "playerCard", True) if c.isVisible()]
    return sorted(cs, key=lambda c: (round(c.mapToScene(QPointF(0, 0)).y()), c.mapToScene(QPointF(0, 0)).x()))
def click_item(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); h.click(p.x(), p.y())
def texts(it, out):
    for c in it.childItems():
        t = c.property("text")
        if isinstance(t, str): out.append(t)
        texts(c, out)
    return out

# Tablettes : grille de cartes (4 joueurs actifs).
for name, w, hh, cols in [("tablette-paysage", 1024, 768, 2), ("tablette-portrait", 800, 1280, 2)]:
    h = make(w, hh); pg = page(h)
    check(pg.property("gridMode") is True and F(h, "playerGrid").isVisible(), f"{name} : grille affichée")
    cs = cards(h)
    check(len(cs) == 4, f"{name} : 4 cartes (joueurs actifs) -> {len(cs)}")
    xs = sorted(set(round(c.mapToScene(QPointF(0, 0)).x()) for c in cs))
    check(len(xs) >= cols, f"{name} : au moins {cols} colonnes ({len(xs)})")
    check(all(c.mapToScene(QPointF(c.width(), 0)).x() <= w - 8 for c in cs), f"{name} : cartes dans la fenêtre")
    shot(h, f"sq_{name}")
    click_item(h, cs[1]); h.spin(500)
    check(pg.property("editing") is True and pg.property("currentId") == 2, f"{name} : clic -> fiche du bon joueur ({pg.property('currentId')})")
    check(not F(h, "playerGrid").isVisible(), f"{name} : grille masquée pendant la fiche")
    shot(h, f"sq_{name}_fiche")
    QMetaObject.invokeMethod(pg, "goBack"); h.spin(300)
    check(pg.property("editing") is False and F(h, "playerGrid").isVisible(), f"{name} : retour -> grille")

# Bureau : liste + vue d'ensemble.
h = make(1280, 800); pg = page(h); ov = F(h, "overviewPane")
check(not pg.property("gridMode") and ov.isVisible(), "bureau : vue d'ensemble affichée, pas de grille")
t = texts(ov, [])
check("4" in t, "bureau : 4 joueurs actifs")
check(any(x.endswith(" ans") for x in t), "bureau : âge moyen affiché")
check("75 %" in t, f"bureau : présence moyenne (95+50+80)/3 = 75 % ({[x for x in t if '%' in x]})")
check("Lucas Silva" in t and "Marco Reus" in t, "bureau : meilleure présence listée")
shot(h, "sq_bureau")
# liste côte à côte : ouverture d'une fiche masque la vue d'ensemble
pg.setProperty("editing", True); h.spin(200)
check(not ov.isVisible(), "bureau : fiche ouverte -> vue d'ensemble masquée")
shot(h, "sq_bureau_fiche")
pg.setProperty("editing", False); h.spin(200)

# Téléphone (portrait et paysage) : liste, pas de grille ni de vue d'ensemble.
for name, w, hh in [("telephone", 412, 892), ("telephone-paysage", 844, 390)]:
    h = make(w, hh); g = F(h, "playerGrid")
    check(not g.isVisible() and len(cards(h)) == 0, f"{name} : pas de grille")
    check(not F(h, "overviewPane").isVisible(), f"{name} : pas de vue d'ensemble")
    if name == "telephone": shot(h, "sq_telephone")

# États vides.
h = make(1280, 800, empty=True); ov = F(h, "overviewPane"); t = texts(ov, [])
check(any("Choisissez un joueur" in x for x in t), "bureau sans joueur : invite affichée (pas de statistiques)")
h = make(1024, 768, empty=True); t = texts(F(h, "playerGrid").parentItem(), [])
check(any("Aucun joueur" in x for x in t), "tablette sans joueur : état vide « Aucun joueur »")
shot(h, "sq_tablette_vide")
h = make(1024, 768, noteam=True); t = texts(F(h, "playerGrid").parentItem(), [])
check(any("Créez votre équipe" in x for x in t), "tablette sans équipe : « Créez votre équipe »")

# Filtre.
h = make(1024, 768); pg = page(h)
pg.setProperty("query", "reus"); h.spin(300)
check(len(cards(h)) == 1, f"filtre « reus » -> 1 carte ({len(cards(h))})")
pg.setProperty("query", ""); pg.setProperty("positionFilter", "MID"); h.spin(300)
check(len(cards(h)) == 1, f"filtre poste MID (actifs) -> 1 carte ({len(cards(h))})")
pg.setProperty("positionFilter", ""); pg.setProperty("showInactive", True); h.spin(400)
check(len(cards(h)) == 5, f"joueurs inactifs inclus -> 5 cartes ({len(cards(h))})")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
