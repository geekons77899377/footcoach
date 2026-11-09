# Fiche d'un exercice : section « Terrain » (miniature, ouverture dans la section Tactique, création à la demande).
# Backend SIMULÉ : le C++ réel est testé par tst_backend (exerciseBoard, createExerciseBoard).
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, QPointF

SCENE = '{"version":1,"pitch":"half","items":[{"kind":"player","x":0.5,"y":0.5,"label":"1"}],"arrows":[]}'

class B6(B2):
    created = []
    boards_of = {10: 501}                 # seul l'exercice 10 a un terrain de référence
    @Slot(int, result="QVariantMap")
    def exerciseBoard(self, ex):
        b = self.boards_of.get(ex, 0)
        return {"boardId": b, "title": "T", "readable": True, "json": SCENE} if b else {"boardId": 0}
    @Slot(int, result="QVariantMap")
    def createExerciseBoard(self, ex):
        B6.created.append(ex)
        self.boards_of[ex] = 600 + ex
        return {"ok": True, "boardId": 600 + ex, "created": True}
    @Slot(int, result="QVariantMap")
    def board(self, i):
        return {"id": i, "title": "T", "readable": True, "error": "", "json": SCENE}

def F(h, name):
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop()
        if it.objectName() == name: return it
        todo.extend(it.childItems())
    return None
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)
def click_item(h, it):
    # Le bouton peut être sous le bord visible de la fiche (défilement) : on déclenche son signal plutôt que la souris.
    QMetaObject.invokeMethod(it, "clicked")
def section(h): return h.win.property("section")

for name, w, hh in [("bureau", 1280, 800), ("telephone", 412, 892)]:
    B6.created = []; B6.boards_of = {10: 501}
    B2.mode = "now"; b = B6(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", -1)); h.spin(600)
    # Exercice 10 : avec terrain.
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 10)); h.spin(600)
    th = F(h, "exerciseBoardThumb")
    check(th is not None and th.isVisible(), f"{name} : miniature du terrain affichée pour l'exercice 10")
    btn = F(h, "openExerciseBoard")
    check(btn is not None and btn.isVisible(), f"{name} : bouton « Ouvrir le terrain »")
    check(F(h, "createExerciseBoard") is None or not F(h, "createExerciseBoard").isVisible(), f"{name} : pas de bouton de création")
    click_item(h, btn); h.spin(600)
    check(section(h) == "tactics", f"{name} : le bouton ouvre la section Tactique ({section(h)})")
    # Exercice 11 : sans terrain -> création puis ouverture.
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 11)); h.spin(600)
    check(F(h, "exerciseBoardThumb") is not None and not F(h, "exerciseBoardThumb").isVisible(), f"{name} : pas de miniature sans terrain")
    cb = F(h, "createExerciseBoard")
    check(cb is not None and cb.isVisible(), f"{name} : bouton « Créer le terrain »")
    click_item(h, cb); h.spin(600)
    check(B6.created == [11], f"{name} : création demandée pour l'exercice 11 ({B6.created})")
    check(section(h) == "tactics", f"{name} : la création ouvre le terrain dans Tactique ({section(h)})")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
