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

# Formation : les titulaires vont au poste correspondant à leur poste (GK, DEF, MID, FWD) ; remplaçants jamais placés ;
# plusieurs titulaires au même poste : pied fort puis prénom alphabétique.
fs = find(h, lambda o: o.metaObject().className().startswith("FormationSheet"))
def P(name, position, foot="", role="starter"):
    return {"name": name, "firstName": name, "lastName": "", "position": position, "foot": foot, "role": role}
def formation_labels(spec, name="4-4-2"):
    be.setProperty("squad", [dict(p, id=i + 1) for i, p in enumerate(spec)]); h.spin(100)
    QMetaObject.invokeMethod(bm, "clearAll") if False else None
    QMetaObject.invokeMethod(fs, "picked", Q_ARG("QString", name), Q_ARG("bool", False)); h.spin(400)
    return {i["label"]: (round(i["x"], 2), round(i["y"], 2)) for i in board_items() if i["kind"] == "player"}
# La liste est mélangée volontairement : le poste décide, pas l'ordre.
full = [P("Att1", "FWD"), P("Def1", "DEF"), P("Mil1", "MID"), P("Gar", "GK"), P("Def2", "DEF"), P("Mil2", "MID"),
        P("Def3", "DEF"), P("Att2", "FWD"), P("Mil3", "MID"), P("Def4", "DEF"), P("Mil4", "MID")]
pos = formation_labels(full)
check(len(pos) == 11, f"11 titulaires, 11 joueurs posés : {len(pos)}")
check(pos["Gar"][0] < 0.1, f"le gardien est au but : {pos['Gar']}")
check(all(pos[n][0] < 0.3 for n in ("Def1", "Def2", "Def3", "Def4")), "les défenseurs sont dans la ligne défensive")
check(all(0.3 < pos[n][0] < 0.55 for n in ("Mil1", "Mil2", "Mil3", "Mil4")), "les milieux sont dans la ligne du milieu")
check(all(pos[n][0] > 0.6 for n in ("Att1", "Att2")), "les attaquants sont en pointe")
# Remplaçants : jamais placés, même au bon poste et même s'il manque un titulaire.
sq = [P("Gar", "GK"), P("Def1", "DEF"), P("Remp", "DEF", role="substitute"), P("Remp2", "GK", role="substitute")]
pos = formation_labels(sq)
check(set(pos) == {"Gar", "Def1"}, f"les remplaçants ne sont pas placés : {sorted(pos)}")
# Plusieurs titulaires au même poste : le pied fort décide (droitier à droite, gaucher à gauche), puis le prénom.
defs = [P("Zoe", "DEF", "R"), P("Yan", "DEF", "L"), P("Xav", "DEF", "R"), P("Wil", "DEF", "L"), P("Vic", "DEF", "B"), P("Gar", "GK")]
pos = formation_labels(defs)
right_back = [n for n, p in pos.items() if p[1] > 0.8 and p[0] < 0.3]
left_back = [n for n, p in pos.items() if p[1] < 0.2 and p[0] < 0.3]
check(right_back == ["Xav"], f"latéral droit : un droitier, le premier par prénom (Xav avant Zoe) : {right_back}")
check(left_back == ["Wil"], f"latéral gauche : un gaucher, le premier par prénom (Wil avant Yan) : {left_back}")
line = lambda p_: sorted(n for n, q in p_.items() if q[0] < 0.3 and n != "Gar")
check(line(pos) == ["Wil", "Xav", "Yan", "Zoe"], f"défense : latéraux Xav (D) et Wil (G), axe droit Zoe (D) et axe gauche Yan (G) : {line(pos)}")
check(pos["Zoe"][1] > 0.5 and pos["Yan"][1] < 0.5, "axe : le droitier côté droit, le gaucher côté gauche")
check(pos["Vic"][0] > 0.3, f"Vic (« les deux », 5e défenseur) occupe une case libre hors de la défense : {pos['Vic']}")
# À pied fort identique ou inconnu : ordre alphabétique des prénoms (accents ignorés), pas l'ordre de la liste.
same = [P("Léo", "DEF"), P("Éric", "DEF"), P("Adam", "DEF"), P("Zack", "DEF"), P("Bob", "DEF"), P("Gar", "GK")]
pos = formation_labels(same)
check(line(pos) == ["Adam", "Bob", "Léo", "Éric"], f"pieds inconnus : défense = Adam, Bob, Éric, Léo (prénom, accents ignorés) : {line(pos)}")
check(pos["Zack"][0] > 0.3, "pieds inconnus : Zack, dernier par prénom, hors de la défense")
# Un « les deux pieds » joue des deux côtés, mais après le pied adapté.
mix = [P("Bi", "DEF", "B"), P("Dr", "DEF", "R"), P("Ga", "DEF", "L"), P("Ax", "DEF", "B"), P("Gar", "GK")]
pos = formation_labels(mix)
check([n for n, p in pos.items() if p[1] > 0.8 and p[0] < 0.3] == ["Dr"] and [n for n, p in pos.items() if p[1] < 0.2 and p[0] < 0.3] == ["Ga"], "pied adapté d'abord, « les deux » ensuite")
# 4-2-3-1 : 1 seul attaquant ; le second attaquant (poste sans place) prend un poste restant.
pos = formation_labels(full, "4-2-3-1")
check(pos["Gar"][0] < 0.1 and pos["Att1"][0] > 0.7, f"4-2-3-1 : gardien au but, premier attaquant en pointe : {pos.get('Att1')}")
check(len(pos) == 11 and 0.3 < pos["Att2"][0] < 0.65, f"4-2-3-1 : le 2e attaquant (poste sans place) prend un poste restant : {pos.get('Att2')}")
# Postes absents : joueurs sans poste placés dans les cases libres, jamais au but ; gardien absent : but vide.
pos = formation_labels([P("A", ""), P("B", ""), P("C", "DEF")])
check(len(pos) == 3 and all(p[0] > 0.1 for p in pos.values()), f"sans poste ni gardien : placés dans le champ, but vide : {pos}")
check(pos["C"][0] < 0.3, "le défenseur reste en défense")
# Deux gardiens : le second ne va pas dans le champ.
pos = formation_labels([P("G1", "GK"), P("G2", "GK"), P("D", "DEF")])
check(set(pos) == {"G1", "D"}, f"deux gardiens : un seul placé : {sorted(pos)}")

# Foot à 8 : formations à 8 joueurs (gardien + 7) et autant d'adversaires que de joueurs, jamais 11.
check(list(fs.property("names")) == ["4-4-2", "4-3-3", "3-5-2", "4-2-3-1"], f"par défaut : formations à 11 : {list(fs.property('names'))}")
fs.setProperty("perSide", 8); h.spin(100)
check(list(fs.property("names")) == ["3-3-1", "2-3-2", "3-2-2", "2-4-1"], f"foot à 8 : formations à 8 : {list(fs.property('names'))}")
eight = [P("Gar", "GK"), P("D1", "DEF"), P("D2", "DEF"), P("D3", "DEF"), P("M1", "MID"), P("M2", "MID"), P("M3", "MID"), P("A1", "FWD")]
be.setProperty("squad", [dict(p, id=i + 1) for i, p in enumerate(eight)]); h.spin(100)
QMetaObject.invokeMethod(fs, "picked", Q_ARG("QString", "3-3-1"), Q_ARG("bool", True)); h.spin(400)
own = [i for i in board_items() if i["kind"] == "player"]; opp = [i for i in board_items() if i["kind"] == "opponent"]
check(len(own) == 8 and len(opp) == 8, f"3-3-1 avec adversaires : 8 joueurs et 8 adversaires ({len(own)} / {len(opp)})")
pos8 = {i["label"]: i["x"] for i in own}
check(pos8["Gar"] < 0.1 and pos8["A1"] > 0.6 and all(pos8[n] < 0.3 for n in ("D1", "D2", "D3")), f"3-3-1 : gardien, 3 défenseurs, attaquant seul : {pos8}")
fs.setProperty("perSide", 11); h.spin(100)

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
