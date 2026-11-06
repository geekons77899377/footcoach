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


# --- Texte sous le joueur : rond vide, 24 caractères au plus, joueurs et adversaires ----------------------------------------------------
h, b = make()
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(4)); h.spin(300)
field(h, "opponentField").setProperty("text", "Légende"); call(ed, "save"); h.spin(300)
call(ed, "openBoard"); h.spin(700)
be = find(h, lambda o: o.metaObject().className().startswith("BoardEditor"))
bm = be.findChild(QObject, "board")
add = lambda kind, x, lab: QMetaObject.invokeMethod(bm, "addItem", Q_ARG("QVariant", kind), Q_ARG("QVariant", x), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", lab))
add("player", 0.3, ""); add("opponent", 0.7, ""); add("text", 0.5, "Note libre"); h.spin(300)
def captions():
    out = []
    def walk(it):
        for c in it.childItems():
            if c.objectName() == "tokenCaption" and c.isVisible(): out.append(c)
            walk(c)
    walk(find(h, lambda o: o.objectName() == "boardView"))
    return out
def cap_texts():
    r = []
    for c in captions():
        for t in c.childItems():
            if t.metaObject().className().startswith("QQuickText"): r.append(t.property("text"))
    return sorted(r)
check(captions() == [], "sans texte : aucune légende affichée")
long = "Gardien remplaçant très très long"
QMetaObject.invokeMethod(bm, "setLabel", Q_ARG("QVariant", 0), Q_ARG("QVariant", "  " + long + "  ")); h.spin(200)
QMetaObject.invokeMethod(bm, "setLabel", Q_ARG("QVariant", 1), Q_ARG("QVariant", "Adv. n°9")); h.spin(300)
add("player", 0.5, "Théo Martin"); add("player", 0.15, "Gardien"); add("opponent", 0.85, "N°10 adverse"); h.spin(300)
h.shot("legende_joueur")
exp = sorted([long[:24].strip(), "Adv. n°9", "Théo Martin", "Gardien", "N°10 adverse"])
check(cap_texts() == exp, f"joueur et adversaire : texte sous le rond, 24 caractères au plus : {cap_texts()}")
check(len(long[:24]) == 24 and bm.property("maxPlayerLabel") == 24, "limite de 24 caractères")
import json
d = json.loads(str(QMetaObject.invokeMethod(bm, "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant"))))
labs = [(i["kind"], i.get("label")) for i in d["items"]]
check(labs[:3] == [("player", long[:24].strip()), ("opponent", "Adv. n°9"), ("text", "Note libre")], f"texte enregistré dans le schéma : {labs}")
# La note texte reste libre (300 caractères) et n'a pas de légende.
QMetaObject.invokeMethod(bm, "setLabel", Q_ARG("QVariant", 2), Q_ARG("QVariant", "x" * 400)); h.spin(200)
d = json.loads(str(QMetaObject.invokeMethod(bm, "toJson", Qt.DirectConnection, Q_RETURN_ARG("QVariant"))))
check(len([i for i in d["items"] if i["kind"] == "text"][0]["label"]) == 300 and len(captions()) == 5, "note texte : 300 caractères, pas de légende")
# Ancien schéma : un numéro court s'affiche sous le rond.
QMetaObject.invokeMethod(bm, "setLabel", Q_ARG("QVariant", 0), Q_ARG("QVariant", "9")); h.spin(200)
check("9" in cap_texts(), "ancien schéma (numéro) : affiché sous le rond")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
