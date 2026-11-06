"""L5/L7 : matchs (liste, fiche, convoqués, schéma tactique, garde de sortie) et matchs dans le calendrier.

Backend SIMULÉ (comme les autres tests de ce dossier) : ce test valide l'interface QML (MatchesPage, MatchEditor,
CalendarPage, Main), pas la base SQLite (voir tests/tst_matches.cpp) ; il ne remplace ni un essai sur appareil ni une
validation Qt 6.9.
"""
import sys, os, datetime
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, Q_RETURN_ARG, QObject, Qt, QPointF
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

TODAY = datetime.date.today()
def D(n): return (TODAY + datetime.timedelta(days=n)).strftime("%Y-%m-%d")
def disp(iso): return iso[8:10] + "/" + iso[5:7] + "/" + iso[0:4]

class B6(B2):
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive):
        return [{"id": i, "teamId": 1, "firstName": "P%d" % i, "lastName": "N%d" % i, "fullName": "P%d N%d" % (i, i),
                 "jerseyNumber": i, "active": True, "position": "", "age": -1} for i in (1, 2, 3)]
    @Slot(result="QVariantList")
    def teams(self):
        return [{"id": 1, "clubId": 1, "clubName": "Mon club", "name": "Senior A", "category": "Seniors", "playerCount": 3}]
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b): return []
    @Slot(int, str, str, result="QStringList")
    def sessionDates(self, team, a, b): return []

def make():
    B6.mode = "none"; b = B6(); b.keep = []
    b._club_init()
    h = Harness(412, 892, backend=b)
    h.spin(500)
    return h, b
def go(h, sec, item=-1, deep=False):
    QMetaObject.invokeMethod(h.win, "goFrom" if deep else "go", Q_ARG("QVariant", sec), Q_ARG("QVariant", item)); h.spin(500)
def back(h):
    r = QMetaObject.invokeMethod(h.win, "handleBack", Qt.DirectConnection, Q_RETURN_ARG("QVariant")); h.spin(500); return r
def section(h): return h.win.property("section")
def find(h, pred):
    for o in h.win.findChildren(QObject):
        try:
            if pred(o): return o
        except Exception: pass
def cls(h, prefix): return find(h, lambda o: o.metaObject().className().startswith(prefix))
def named(h, name): return find(h, lambda o: o.objectName() == name)
def call(o, m, *a): QMetaObject.invokeMethod(o, m, *[Q_ARG("QVariant", x) for x in a])
def field(h, name): return named(h, name)
def items(h, name, visible=True):
    # Les éléments créés par un Repeater ne sont pas des enfants QObject : on parcourt l'arbre visuel.
    todo = [h.win.contentItem()]; out = []
    while todo:
        it = todo.pop()
        if it.objectName() == name and (it.isVisible() or not visible): out.append(it)
        todo.extend(it.childItems())
    return out
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v


# --- Terrain d'un match : seuls les convoqués sont posables ---------------------------------------------------------------------------
import json
from PySide6.QtCore import Qt, Q_RETURN_ARG
h, b = make()
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(4)); h.spin(300)
field(h, "opponentField").setProperty("text", "Rivaux")
call(ed, "setRole", 1, "starter"); call(ed, "setRole", 2, "substitute"); call(ed, "setRole", 3, "starter")
call(ed, "save"); h.spin(300)
call(ed, "openBoard"); h.spin(700)
be = find(h, lambda o: o.metaObject().className().startswith("BoardEditor"))
bm = be.findChild(QObject, "board")
view = find(h, lambda o: o.objectName() == "boardView")
def board_items():
    d = json.loads(str(QMetaObject.invokeMethod(bm, "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant"))))
    return d["items"]
def sheet(name): return find(h, lambda o: o.objectName() == name)
def place(x, y):
    QMetaObject.invokeMethod(view, "placeRequested", Q_ARG("qreal", x), Q_ARG("qreal", y)); h.spin(300)
squad = be.property("squad").toVariant()
check(be.property("squadMode") is True, "le terrain d'un match est en mode effectif")
check([s["name"] for s in squad] == ["P1 N1", "P3 N3", "P2 N2"], f"convoqués, titulaires d'abord : {[s['name'] for s in squad]}")

be.setProperty("tool", "player"); h.spin(100)
place(0.4, 0.5)
ss = sheet("squadSheet")
check(ss.property("opened") is True, "« Joueur » ouvre la liste des convoqués au lieu d'ajouter un rond anonyme")
check(len(board_items()) == 0, "rien n'est posé avant le choix")
texts_ = [m["text"] for m in ss.property("model").toVariant()]
check(texts_ == ["P1 N1 · Titulaire", "P3 N3 · Titulaire", "P2 N2 · Remplaçant"], f"liste : convoqués seulement, avec leur rôle : {texts_}")
QMetaObject.invokeMethod(ss, "chosen", Q_ARG("QString", "1")); h.spin(300)
it = board_items()
check(len(it) == 1 and it[0]["kind"] == "player" and it[0]["label"] == "P3 N3", f"le joueur choisi est posé avec son nom : {it}")
place(0.6, 0.5); QMetaObject.invokeMethod(sheet("squadSheet"), "chosen", Q_ARG("QString", "1")); h.spin(300)
check(len(board_items()) == 2, "un même joueur peut être posé plusieurs fois")

# Nom trop long : prénom + initiale (24 caractères au plus).
long = {"id": 9, "name": "Jean-Baptiste Montmorency-Delacroix", "firstName": "Jean-Baptiste", "lastName": "Montmorency-Delacroix", "role": "starter"}
lab = be.metaObject() and QMetaObject.invokeMethod(be, "squadLabel", Qt.DirectConnection, Q_RETURN_ARG("QVariant"), Q_ARG("QVariant", long))
check(str(lab) == "Jean-Baptiste M.", f"nom long : prénom + initiale : {lab}")

# Formation : remplie avec les titulaires convoqués seulement.
fs = find(h, lambda o: o.metaObject().className().startswith("FormationSheet"))
QMetaObject.invokeMethod(fs, "picked", Q_ARG("QString", "4-4-2"), Q_ARG("bool", False)); h.spin(400)
pl = [i["label"] for i in board_items() if i["kind"] == "player"]
check(pl == ["P1 N1", "P3 N3"], f"formation : les 2 titulaires seulement, dans l'ordre, postes en trop vides : {pl}")

# Pas de texte libre pour un joueur : modifier un rond propose les convoqués (remplacement) ; adversaire et note restent libres.
def prompts(): return [o for o in h.win.findChildren(QObject) if o.metaObject().className().startswith("PromptDialog") and o.property("opened")]
QMetaObject.invokeMethod(be, "editLabel", Q_ARG("QVariant", 0)); h.spin(300)
ss = sheet("squadSheet")
check(ss.property("opened") is True and ss.property("title") == "Remplacer par" and not prompts(), "modifier un joueur : liste des convoqués, pas de saisie libre")
QMetaObject.invokeMethod(ss, "chosen", Q_ARG("QString", "0")); h.spin(300)
check(board_items()[0]["label"] == "P1 N1" and board_items()[0]["kind"] == "player", f"remplacement : le rond porte un autre convoqué : {board_items()[0]}")
n0 = len(board_items())
QMetaObject.invokeMethod(bm, "addItem", Q_ARG("QVariant", "opponent"), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", 0.2), Q_ARG("QVariant", "")); h.spin(200)
QMetaObject.invokeMethod(be, "editLabel", Q_ARG("QVariant", n0)); h.spin(300)
check(len(prompts()) == 1, "adversaire : texte libre toujours permis")

# Un joueur retiré des convoqués n'est plus proposé (la liste suit la fiche du match, même avant enregistrement).
call(ed, "setRole", 2, ""); h.spin(200)
check([s_["name"] for s_ in be.property("squad").toVariant()] == ["P1 N1", "P3 N3"], "joueur retiré des convoqués : plus proposé")

# Aucun convoqué : message, pas de liste, pas de rond.
call(page, "closeEditor") if False else None
h2, b2 = make()
go(h2, "matches"); page2 = cls(h2, "MatchesPage"); ed2 = cls(h2, "MatchEditor")
call(page2, "newMatch", D(5)); h2.spin(300)
field(h2, "opponentField").setProperty("text", "Sans convoqués"); call(ed2, "save"); h2.spin(300)
call(ed2, "openBoard"); h2.spin(700)
be2 = find(h2, lambda o: o.metaObject().className().startswith("BoardEditor")); bm2 = be2.findChild(QObject, "board")
view2 = find(h2, lambda o: o.objectName() == "boardView")
be2.setProperty("tool", "player"); h2.spin(100)
QMetaObject.invokeMethod(view2, "placeRequested", Q_ARG("qreal", 0.5), Q_ARG("qreal", 0.5)); h2.spin(300)
ss2 = find(h2, lambda o: o.objectName() == "squadSheet")
check(ss2.property("opened") is not True, "aucun convoqué : pas de liste")
d2 = json.loads(str(QMetaObject.invokeMethod(bm2, "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant"))))
check(len(d2["items"]) == 0, "aucun convoqué : aucun rond anonyme ajouté")
fs2 = find(h2, lambda o: o.metaObject().className().startswith("FormationSheet"))
QMetaObject.invokeMethod(fs2, "picked", Q_ARG("QString", "4-4-2"), Q_ARG("bool", False)); h2.spin(300)
d2 = json.loads(str(QMetaObject.invokeMethod(bm2, "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant"))))
check(len(d2["items"]) == 0, "aucun titulaire : la formation ne pose rien")
h2.shot("terrain_match_sans_convoque")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
