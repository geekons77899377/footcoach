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



# --- Barre d'actions de sélection : seulement avec l'outil de sélection, déplaçable --------------------------------------------------
import json
from PySide6.QtCore import Qt, Q_RETURN_ARG
h, b = make()
go(h, "matches"); page = cls(h, "MatchesPage"); ed = cls(h, "MatchEditor")
call(page, "newMatch", D(4)); h.spin(300)
field(h, "opponentField").setProperty("text", "Rivaux"); call(ed, "setRole", 1, "starter"); call(ed, "save"); h.spin(300)
call(ed, "openBoard"); h.spin(700)
be = find(h, lambda o: o.metaObject().className().startswith("BoardEditor"))
bm = be.findChild(QObject, "board")
bar = be.findChild(QObject, "contextBar")
grip = be.findChild(QObject, "contextGrip")
QMetaObject.invokeMethod(bm, "addItem", Q_ARG("QVariant", "opponent"), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", 0.5), Q_ARG("QVariant", "")); h.spin(200)
be.setProperty("tool", "select"); h.spin(100)
QMetaObject.invokeMethod(bm, "selectItem", Q_ARG("QVariant", 0), Q_ARG("QVariant", False)); h.spin(300)
check(bm.property("selCount") == 1 and bar.isVisible(), "outil de sélection + élément sélectionné : barre d'actions visible")
be.setProperty("tool", "multi"); h.spin(200)
check(bar.isVisible() and bm.property("selCount") == 1, "sélection multiple : la barre reste visible, sélection conservée")
for t in ("player", "opponent", "ball", "cone", "text"):
    be.setProperty("tool", "select"); h.spin(80)
    QMetaObject.invokeMethod(bm, "selectItem", Q_ARG("QVariant", 0), Q_ARG("QVariant", False)); h.spin(120)
    be.setProperty("tool", t); h.spin(200)
    check(not bar.isVisible() and bm.property("selCount") == 0, f"outil « {t} » : barre masquée et sélection levée")
be.setProperty("tool", "select"); h.spin(100)
check(not bar.isVisible(), "retour à la sélection : rien n'est sélectionné, barre masquée")

# Déplacement par la poignée.
QMetaObject.invokeMethod(bm, "selectItem", Q_ARG("QVariant", 0), Q_ARG("QVariant", False)); h.spin(300)
def center(it):
    p = it.mapToScene(QPointF(it.width() / 2, it.height() / 2)); return p.x(), p.y()
def bar_pos():
    p = bar.mapToScene(QPointF(0, 0)); return p.x(), p.y()
x0, y0 = bar_pos(); gx, gy = center(grip)
h.drag(gx, gy, gx - 90, gy - 220); h.spin(300)
x1, y1 = bar_pos()
check(abs((x1 - x0) + 90) < 6 and abs((y1 - y0) + 220) < 6, f"la poignée déplace la barre : ({x0:.0f},{y0:.0f}) -> ({x1:.0f},{y1:.0f})")
check(bar.isVisible() and bm.property("selCount") == 1, "après déplacement : barre toujours là, sélection conservée")
# Jamais hors de la vue.
x1, y1 = bar_pos(); gx, gy = center(grip)
h.drag(gx, gy, gx - 3000, gy - 3000); h.spin(300)
x2, y2 = bar_pos(); view = be.findChild(QObject, "boardView"); vp = view.mapToScene(QPointF(0, 0))
check(x2 >= vp.x() - 1 and y2 >= vp.y() - 1, f"limitée à la vue : ({x2:.0f},{y2:.0f}) >= ({vp.x():.0f},{vp.y():.0f})")
gx, gy = center(grip); h.drag(gx, gy, gx + 5000, gy + 5000); h.spin(300)
x3, y3 = bar_pos()
check(x3 + bar.width() <= vp.x() + view.width() + 1 and y3 + bar.height() <= vp.y() + view.height() + 1, "limitée à la vue (bas et droite)")
h.shot("barre_deplacee")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
