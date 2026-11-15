"""Schéma 13 : rubriques de la fiche exercice (Consignes, Évolutions, Critères de réussite, Consigne éducateur) et
fiche exercice PDF. Téléphone et bureau.

Backend SIMULÉ : ce test valide l'interface QML ; la base et le PDF réels sont testés par tests/tst_exercisesheet.cpp.
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG

PORTES = {"id": 10, "title": "Le parcours des portes", "category": "technical", "categoryLabel": "Technique",
          "objective": "Conduite de balle", "durationMin": 17, "playersMin": 8, "playersMax": 12,
          "equipment": "1 ballon par joueur", "description": "Terrain : 20 x 20 m", "archived": False, "usage": 0,
          "categoryId": 1, "categoryName": "U10-U11",
          "instructions": "Conduire librement son ballon.\nTraverser un maximum de portes.",
          "variations": "Varier les directions.", "successCriteria": "Lever la tête.",
          "coachCue": "Regarde devant toi !"}

class B9(B2):
    def __init__(self):
        super().__init__()
        self.saved = []; self.exports = []; self.temp = []
    @Slot(result="QVariantList")
    def exerciseCategories(self): return [{"id": "technical", "text": "Technique"}]
    @Slot(bool, result="QVariantList")
    def exercises(self, a): return [dict(PORTES)]
    @Slot(result="QVariantList")
    def categories(self): return [{"id": 1, "name": "U10-U11", "count": 1}]
    @Slot("QVariantMap", result="QVariantMap")
    def saveExercise(self, e):
        self.saved.append(dict(e))
        if not str(e.get("title", "")).strip():
            return {"ok": False, "id": -1, "error": "Le titre est obligatoire."}
        return {"ok": True, "id": e.get("id", -1) if e.get("id", -1) > 0 else 99}
    @Slot(int, result=str)
    def exerciseSheetFileName(self, i): return "fiche-le-parcours-des-portes.pdf"
    @Slot(str, result=str)
    def challengeSheetTempUrl(self, name):
        self.temp.append(name); return "file:///tmp/" + name
    @Slot(int, str, result="QVariantMap")
    def exportExerciseSheet(self, i, url):
        self.exports.append((i, url)); return {"ok": True, "hasBoard": True, "bytes": 1000}

def walk(h):
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop(); yield it; todo.extend(it.childItems())
def F(h, name):
    for it in walk(h):
        if it.objectName() == name: return it
def form(h):
    for it in walk(h):
        if it.property("categoryOptions") is not None and it.property("exerciseId") is not None: return it
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

for name, w, hh in [("bureau", 1280, 800), ("telephone", 412, 892)]:
    B2.mode = "now"; b = B9(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 10)); h.spin(700)
    f = form(h)
    check(f is not None and f.property("exerciseId") == 10, f"{name} : fiche de l'exercice ouverte")
    fields = {k: F(h, k) for k in ["exerciseInstructions", "exerciseVariations", "exerciseCriteria", "exerciseCoachCue"]}
    check(all(v is not None for v in fields.values()), f"{name} : les 4 rubriques sont dans la fiche")
    check(fields["exerciseInstructions"].property("text") == PORTES["instructions"], f"{name} : consignes chargées (une par ligne)")
    check(fields["exerciseCoachCue"].property("text") == "Regarde devant toi !", f"{name} : consigne éducateur chargée")
    check(fields["exerciseInstructions"].property("label") == "Consignes" and fields["exerciseCriteria"].property("label") == "Critères de réussite",
          f"{name} : libellés des rubriques")
    if name == "telephone":
        f.findChild(QObject, "") if False else None
        fls = [o for o in walk(h) if o.metaObject().className() == "QQuickFlickable" and f.isAncestorOf(o)]
        fl = max(fls, key=lambda o: o.property("contentHeight"))
        fl.setProperty("contentY", max(0, fl.property("contentHeight") - fl.property("height")) - 80); h.spin(300)
        h.shot("fiche_exo_rubriques")

    # Enregistrer : les 4 rubriques partent avec l'exercice.
    fields["exerciseVariations"].setProperty("text", "Varier les directions.\nAccélérer après chaque porte.")
    QMetaObject.invokeMethod(f, "save"); h.spin(400)
    last = b.saved[-1] if b.saved else {}
    check(last.get("variations") == "Varier les directions.\nAccélérer après chaque porte." and last.get("successCriteria") == "Lever la tête."
          and last.get("coachCue") == "Regarde devant toi !" and "instructions" in last, f"{name} : les rubriques sont enregistrées ({ {k: last.get(k) for k in ('instructions','variations','successCriteria','coachCue')} })")

    # Fiche PDF : bouton visible, enregistre sans quitter la fiche puis crée la fiche.
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 10)); h.spin(500)
    f = form(h)
    f.setProperty("dryRun", True)
    btn = F(h, "exerciseSheetButton")
    check(btn is not None and btn.property("visible") is True, f"{name} : bouton « Fiche exercice (PDF) » visible")
    n = len(b.saved)
    QMetaObject.invokeMethod(btn, "clicked"); h.spin(500)
    check(len(b.saved) == n + 1, f"{name} : la saisie est enregistrée avant la fiche")
    check(b.exports and b.exports[-1] == (10, "file:///tmp/fiche-le-parcours-des-portes.pdf"), f"{name} : fiche créée dans le cache ({b.exports[-1:]})")
    check(f.property("lastSheetUrl") == "file:///tmp/fiche-le-parcours-des-portes.pdf", f"{name} : fiche ouverte (simulé)")
    check(form(h) is not None and form(h).property("visible") is True, f"{name} : la fiche exercice reste ouverte")
    # Titre vidé : enregistrement refusé, aucune fiche.
    F(h, "") if False else None
    tf = [o for o in f.findChildren(QObject) if o.metaObject().className().startswith("FormField") and o.property("label") == "Titre"][0]
    tf.setProperty("text", "")
    k = len(b.exports)
    QMetaObject.invokeMethod(f, "makeSheet"); h.spin(300)
    check(len(b.exports) == k and "obligatoire" in f.property("errorText"), f"{name} : titre vide -> pas de fiche, erreur affichée")
    bad = [x for x in h.warnings if "ExerciseForm" in x]
    check(not bad, f"{name} : aucun avertissement QML dans la fiche exercice ({bad[:2]})")

print("\n%s" % ("ECHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
