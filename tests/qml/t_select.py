# Effectif : sélection multiple (appui long, cases à cocher, « Tous », poubelle, confirmation, retour) ;
# Données : « Tout effacer » (avertissement avec le contenu, mot EFFACER à taper). Backend simulé : le C++ réel est
# testé par tst_reset.
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_squad.py")).read().split("def make(")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF, Qt
from PySide6.QtTest import QTest

class B7(B4):
    two_teams = True
    def __init__(self):
        super().__init__(); self.calls = []
    @Slot(result="QVariantList")
    def teams(self):
        t = [{"id": 1, "name": "Senior A", "category": "Seniors", "playerCount": 4, "clubName": "Mon club"}]
        if B7.two_teams: t.append({"id": 2, "name": "U13", "category": "U13", "playerCount": 12, "clubName": "Mon club"})
        return t
    @Slot(bool, result="QVariantList")
    def allPlayers(self, inactive):
        self.calls.append(("allPlayers", inactive))
        a = [dict(p, teamId=1, teamName="Senior A") for p in B4.players(self, 1, inactive)]
        b = [dict(a[0], id=50, fullName="Zoé Perdue", firstName="Zoé", lastName="Perdue", teamId=2, teamName="U13")]
        return a + b
    @Slot("QVariantList", int, result="QVariantMap")
    def movePlayers(self, ids, team):
        self.calls.append(("movePlayers", sorted(int(i) for i in ids), team))
        return {"ok": True, "error": "", "moved": len(ids) - 1, "numbersCleared": 1, "skipped": ["Yann Portier"]}
    @Slot("QVariantList", result="QVariantMap")
    def deletePlayers(self, ids):
        self.calls.append(("deletePlayers", sorted(int(i) for i in ids)))
        return {"ok": True, "error": "", "removed": len(ids)}
    @Slot(result="QVariantMap")
    def dataCounts(self):
        return {"teams": 2, "players": 31, "sessions": 64, "exercises": 46, "boards": 30}
    @Slot(result="QVariantMap")
    def resetAllData(self):
        self.calls.append(("reset",)); return {"ok": True, "error": "", "backupName": "avant-effacement-x-001.footcoach"}
    @Slot(result=str)
    def suggestedExportName(self): return "footcoach.footcoach"

def make7(w, hh, where="squad", two=True):
    B7.two_teams = two
    B4.empty = False; B4.noteam = False
    b = B7(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", where), Q_ARG("QVariant", -1)); h.spin(700)
    return h, b

def click_item(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); h.click(p.x(), p.y())
def texts(it, out):
    for c in it.childItems():
        t = c.property("text")
        if isinstance(t, str): out.append(t)
        texts(c, out)
    return out

def long_press(h, it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)).toPoint()
    QTest.mousePress(h.win, Qt.LeftButton, Qt.NoModifier, p); h.spin(800)
    QTest.mouseRelease(h.win, Qt.LeftButton, Qt.NoModifier, p); h.spin(300)

def objs(h, pred):
    out, todo = [], [h.win]
    while todo:
        o = todo.pop()
        if pred(o): out.append(o)
        todo.extend(o.children())
    return out

def open_dialog(h):
    ds = [o for o in objs(h, lambda o: o.property("okText") is not None and o.property("message") is not None) if o.property("opened")]
    return ds[0] if ds else None

def accept(h, d, text=""):
    QMetaObject.invokeMethod(d, "close"); QMetaObject.invokeMethod(d, "accepted", Q_ARG("QString", text)); h.spin(500)

def squad_page(h):
    it = F(h, "selectionBar")
    while it is not None and it.property("selecting") is None: it = it.parentItem()
    return it

def rows(h, name):
    cs = [c for c in F(h, name, True) if c.isVisible()]
    return sorted(cs, key=lambda c: (round(c.mapToScene(QPointF(0, 0)).y()), c.mapToScene(QPointF(0, 0)).x()))

def marks(it):
    out, todo = [], [it]
    while todo:
        c = todo.pop()
        if c.property("checked") is not None and c.property("radius") == 14: out.append(c)
        todo.extend(c.childItems())
    return out

for name, w, hh, rowname in [("telephone", 412, 892, "playerRow"), ("tablette", 1024, 768, "playerCard"), ("bureau", 1280, 800, "playerRow")]:
    h, b = make7(w, hh); pg = squad_page(h)
    check(pg is not None and not pg.property("selecting"), f"{name} : pas de sélection au départ")
    rs = rows(h, rowname)
    check(len(rs) == 4, f"{name} : 4 joueurs actifs ({len(rs)})")
    check(not any(m.isVisible() for r in rs for m in marks(r)), f"{name} : pas de case à cocher hors sélection")
    long_press(h, rs[1])
    check(pg.property("selecting") and pg.property("selectedCount") == 1, f"{name} : appui long -> sélection d'1 joueur")
    check(not pg.property("editing"), f"{name} : l'appui long n'ouvre pas la fiche")
    bar = F(h, "selectionBar")
    check(bar.isVisible(), f"{name} : barre de sélection affichée")
    cnt = F(h, "selectionCount").property("text")
    check(cnt == "1 sélectionné", f"{name} : compteur : {cnt!r}")
    rs = rows(h, rowname)
    vis_marks = [m for r in rs for m in marks(r) if m.isVisible()]
    check(len(vis_marks) == 4 and sum(1 for m in vis_marks if m.property("checked")) == 1, f"{name} : cases à cocher sur tous les joueurs, 1 cochée")
    click_item(h, rs[2]); click_item(h, rs[3])
    check(pg.property("selectedCount") == 3 and not pg.property("editing"), f"{name} : toucher coche sans ouvrir la fiche")
    click_item(h, rs[3])
    check(pg.property("selectedCount") == 2, f"{name} : toucher à nouveau décoche")
    trash = F(h, "deleteSelectedButton")
    tp = trash.mapToScene(QPointF(trash.width(), 0))
    check(trash.isVisible() and trash.property("enabled") and tp.x() <= bar.mapToScene(QPointF(bar.width(), 0)).x(), f"{name} : poubelle en haut, active")
    shot(h, f"select_{name}_selection")

    # « Tous » puis « Aucun ».
    allb = F(h, "selectAllButton")
    click_item(h, allb)
    check(pg.property("selectedCount") == 4 and F(h, "selectionCount").property("text") == "4 sélectionnés", f"{name} : « Tous » coche les 4 joueurs")
    check(texts(allb, []) == ["Aucun"], f"{name} : le bouton devient « Aucun »")
    click_item(h, allb)
    check(pg.property("selectedCount") == 0 and not trash.property("enabled"), f"{name} : « Aucun » décoche, poubelle inactive")
    click_item(h, allb)

    # Poubelle -> confirmation -> deletePlayers.
    click_item(h, trash); h.spin(300)
    d = open_dialog(h)
    check(d is not None and d.property("title") == "Supprimer 4 joueurs ?" and d.property("danger"), f"{name} : confirmation : {d.property('title') if d else None!r}")
    check(d is not None and "désactivez plutôt" in d.property("message"), f"{name} : conseil de désactivation rappelé")
    check(not b.calls, f"{name} : rien supprimé avant confirmation")
    shot(h, f"select_{name}_confirmation")
    if d: accept(h, d)
    check(b.calls == [("deletePlayers", [1, 2, 3, 4])], f"{name} : deletePlayers([1, 2, 3, 4]) : {b.calls}")
    check(not pg.property("selecting"), f"{name} : sortie de la sélection après suppression")

    # Retour (Android) : quitte la sélection sans rien supprimer.
    long_press(h, rows(h, rowname)[0])
    check(pg.property("selecting"), f"{name} : nouvel appui long")
    QMetaObject.invokeMethod(pg, "goBack"); h.spin(300)
    check(not pg.property("selecting") and pg.property("selectedCount") == 0 and len(b.calls) == 1, f"{name} : retour -> sélection annulée")
    # Croix : idem.
    long_press(h, rows(h, rowname)[0])
    click_item(h, [c for c in objs(h, lambda o: o.property("accessibleName") == "Annuler la sélection")][0])
    check(not pg.property("selecting"), f"{name} : croix -> sélection annulée")
    # Un toucher simple ouvre toujours la fiche.
    click_item(h, rows(h, rowname)[0])
    check(pg.property("editing"), f"{name} : toucher simple hors sélection -> fiche")

# Déplacer vers une autre équipe.
def jsval(v): return v.toVariant() if hasattr(v, "toVariant") else v
for name, w, hh, rowname in [("telephone", 412, 892, "playerRow"), ("bureau", 1280, 800, "playerRow")]:
    h, b = make7(w, hh); pg = squad_page(h)
    long_press(h, rows(h, rowname)[0]); click_item(h, rows(h, rowname)[3])
    mv = F(h, "moveSelectedButton"); bar = F(h, "selectionBar")
    check(mv.isVisible() and mv.property("enabled"), f"{name} : bouton Déplacer visible et actif")
    xs = [F(h, n).mapToScene(QPointF(0, 0)).x() for n in ("selectAllButton", "moveSelectedButton", "deleteSelectedButton")]
    check(xs == sorted(xs) and F(h, "deleteSelectedButton").mapToScene(QPointF(48, 0)).x() <= w, f"{name} : Tous, Déplacer, poubelle alignés dans la barre {xs}")
    cnt = F(h, "selectionCount")
    check(cnt.property("truncated") is False, f"{name} : compteur lisible en entier ({cnt.property('text')!r})")
    shot(h, f"move_{name}_barre")
    click_item(h, mv); h.spin(300)
    sh = F(h, "moveSheet") or [o for o in objs(h, lambda o: o.objectName() == "moveSheet")][0]
    labels = [m["text"] for m in jsval(sh.property("model"))]
    check(labels == ["U13 · Mon club"], f"{name} : équipes proposées (sans l'équipe actuelle) : {labels}")
    QMetaObject.invokeMethod(sh, "close"); QMetaObject.invokeMethod(sh, "chosen", Q_ARG("QString", "t2")); h.spin(400)
    d = open_dialog(h)
    check(d is not None and d.property("title") == "Déplacer 2 joueurs ?" and "« U13 »" in d.property("message"), f"{name} : confirmation du déplacement")
    check(not b.calls, f"{name} : rien déplacé avant confirmation")
    shot(h, f"move_{name}_confirmation")
    accept(h, d)
    check(b.calls == [("movePlayers", [1, 4], 2)], f"{name} : movePlayers([1, 4], 2) : {b.calls}")
    check(not pg.property("selecting"), f"{name} : sélection terminée")
    i = open_dialog(h)
    m = i.property("message") if i else ""
    check(i is not None and "1 joueur déplacé vers U13 · 1 numéro retiré" in m and "Yann Portier" in m and not i.property("cancelVisible"), f"{name} : compte rendu avec les joueurs non déplacés : {m!r}")
    shot(h, f"move_{name}_compte_rendu")
# Vue « Tous les joueurs » : joueurs de toutes les équipes, avec leur équipe, à répartir par le bouton Déplacer.
h, b = make7(412, 892); pg = squad_page(h)
sheets = [o for o in objs(h, lambda o: o.objectName() == "teamSheet")]
check(len(sheets) >= 1, "feuille « Équipe » trouvée")
ts = sheets[0]
QMetaObject.invokeMethod(ts, "rebuild"); h.spin(100)
labels = [m["text"] for m in jsval(ts.property("model"))]
check(labels[0] == "Tous les joueurs (toutes les équipes)", f"première entrée de la liste des équipes : {labels[:2]}")
QMetaObject.invokeMethod(ts, "chosen", Q_ARG("QString", "all")); h.spin(500)
check(pg.property("allMode") is True and ("allPlayers", False) in b.calls, f"vue « Tous les joueurs » active : {b.calls}")
rs = rows(h, "playerRow")
metas = []
for r in rs: texts(r, metas)
check(any("U13" in t for t in metas) and any("Senior A" in t for t in metas), f"chaque joueur affiche son équipe : {[t for t in metas if 'U13' in t or 'Senior A' in t][:3]}")
check(not F(h, "importButton").isVisible(), "vue « Tous les joueurs » : pas d'import (il faut une équipe)")
long_press(h, rs[-1]); h.spin(200)
mv = F(h, "moveSelectedButton")
check(mv.isVisible() and mv.property("enabled"), "vue « Tous les joueurs » : Déplacer disponible")
click_item(h, mv); h.spin(300)
msh = [o for o in objs(h, lambda o: o.objectName() == "moveSheet")][0]
labels = [m["text"] for m in jsval(msh.property("model"))]
check(labels == ["Senior A · Mon club", "U13 · Mon club"], f"toutes les équipes proposées comme destination : {labels}")
QMetaObject.invokeMethod(msh, "close"); QMetaObject.invokeMethod(msh, "chosen", Q_ARG("QString", "t1")); h.spin(400)
d = open_dialog(h); accept(h, d)
mp = [c for c in b.calls if c[0] == "movePlayers"]
check(len(mp) == 1 and mp[0][2] == 1, f"movePlayers vers Senior A : {mp}")
# Retour à une équipe : la vue normale revient.
QMetaObject.invokeMethod(ts, "rebuild"); h.spin(100)
QMetaObject.invokeMethod(ts, "chosen", Q_ARG("QString", "t2")); h.spin(300)
check(pg.property("allMode") is False and pg.property("teamId") == 2, "choisir une équipe quitte la vue « Tous les joueurs »")

# Une seule équipe : pas de bouton Déplacer.
h, b = make7(412, 892, two=False)
long_press(h, rows(h, "playerRow")[0])
check(not F(h, "moveSelectedButton").isVisible(), "une seule équipe : bouton Déplacer masqué")

# Données : tout effacer.
for name, w, hh in [("telephone", 412, 892), ("bureau", 1280, 800)]:
    h, b = make7(w, hh, "data")
    card = F(h, "resetCard"); btn = F(h, "resetButton")
    check(card is not None and card.isVisible(), f"{name} : carte « Tout effacer » présente")
    check(any("Tout effacer" in t for t in texts(card, [])), f"{name} : titre de la carte")
    QMetaObject.invokeMethod(btn, "clicked"); h.spin(400)
    d = open_dialog(h)
    msg = d.property("message") if d else ""
    check(d is not None and d.property("title") == "Tout effacer ?" and d.property("danger"), f"{name} : avertissement ouvert")
    check("2 équipes, 31 joueurs, 64 séances, 46 exercices, 30 schémas" in msg and "Envoyer mes données" in msg, f"{name} : contenu actuel et conseil de sauvegarde : {msg[:90]!r}")
    shot(h, f"reset_{name}_avertissement")
    accept(h, d)
    d2 = open_dialog(h)
    check(d2 is not None and d2.objectName() == "resetTyped" and d2.property("fieldVisible") and d2.property("okText") == "Tout effacer", f"{name} : 2e étape : mot à taper")
    check(not b.calls, f"{name} : rien effacé après la 1re confirmation")
    shot(h, f"reset_{name}_mot")
    accept(h, d2, "effacé")
    check(not b.calls, f"{name} : mauvais mot -> rien effacé")
    QMetaObject.invokeMethod(btn, "clicked"); h.spin(400); accept(h, open_dialog(h))
    accept(h, open_dialog(h), " effacer ")
    check(b.calls == [("reset",)], f"{name} : mot EFFACER (casse et espaces ignorés) -> resetAllData : {b.calls}")

warn = [x for x in h.warnings if "SquadPage" in x or "DataPage" in x or "SelectMark" in x]
check(not warn, f"aucun avertissement QML : {warn[:3]}")
print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
