# Catégories d'exercices (ex. U10-U11) distinctes de l'« Activité » : filtres de la bibliothèque, choix et création dans la
# fiche. Backend SIMULÉ : le C++ réel est testé par tst_backend (catégories) et tst_dataformat (export/import, migration 009).
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG

def ex(i, t, cat):
    return {"id": i, "title": t, "category": "technical", "categoryLabel": "Technique", "objective": "", "durationMin": 10,
            "playersMin": 0, "playersMax": 0, "equipment": "", "description": "", "archived": False, "usage": 0,
            "categoryId": cat[0], "categoryName": cat[1]}

class B7(B2):
    cats = [{"id": 1, "name": "U10-U11", "count": 2}]
    saved = []
    added = []
    @Slot(result="QVariantList")
    def exerciseCategories(self): return [{"id": "technical", "text": "Technique"}, {"id": "game", "text": "Jeu"}]
    @Slot(bool, result="QVariantList")
    def exercises(self, a): return [ex(10, "Rondo", (1, "U10-U11")), ex(11, "Passes", (1, "U10-U11")), ex(12, "Mon exercice", (0, ""))]
    @Slot(result="QVariantList")
    def categories(self): return list(B7.cats)
    @Slot(str, result="QVariantMap")
    def addCategory(self, name):
        B7.added.append(name)
        n = 2 + len(B7.added)
        B7.cats.append({"id": n, "name": name, "count": 0})
        return {"ok": True, "id": n}
    @Slot("QVariantMap", result="QVariantMap")
    def saveExercise(self, e):
        B7.saved.append(dict(e))
        return {"ok": True, "id": e.get("id", -1) if e.get("id", -1) > 0 else 99}

def walk(h):
    todo = [h.win.contentItem()]
    while todo:
        it = todo.pop()
        yield it
        todo.extend(it.childItems())
def F(h, name):
    for it in walk(h):
        if it.objectName() == name: return it
def page(h):
    for it in walk(h):
        if it.property("activityFilter") is not None and it.property("categoryFilter") is not None and it.property("editing") is not None:
            return it
def form(h):
    for it in walk(h):
        if it.property("categoryOptions") is not None and it.property("exerciseId") is not None: return it
def pv(o, n):
    v = o.property(n)
    return v.toVariant() if hasattr(v, "toVariant") else v
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

for name, w, hh in [("bureau", 1280, 800), ("telephone", 412, 892)]:
    B7.cats = [{"id": 1, "name": "U10-U11", "count": 2}]; B7.saved = []; B7.added = []
    B2.mode = "now"; b = B7(); b.keep = []
    h = Harness(w, hh, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", -1)); h.spin(600)
    p = page(h)
    check(p is not None, f"{name} : page Exercices trouvée")
    row = F(h, "categoryFilterRow")
    check(row is not None and row.isVisible(), f"{name} : rangée de filtre « Catégorie » visible")
    check(row.property("label") == "Catégorie", f"{name} : libellé « Catégorie » ({row.property('label')})")
    act = F(h, "activityFilterRow")
    check(act is not None and act.property("label") == "Activité", f"{name} : libellé « Activité » pour l'ancienne catégorie")
    check(len(pv(p, "shown")) == 3, f"{name} : 3 exercices sans filtre ({len(pv(p, 'shown'))})")
    p.setProperty("categoryFilter", "1"); h.spin(200)
    check(len(pv(p, "shown")) == 2, f"{name} : filtre U10-U11 -> 2 exercices ({len(pv(p, 'shown'))})")
    p.setProperty("activityFilter", "game"); h.spin(200)
    check(len(pv(p, "shown")) == 0, f"{name} : filtres combinés (U10-U11 + Jeu) -> 0")
    p.setProperty("activityFilter", ""); p.setProperty("categoryFilter", ""); h.spin(200)

    # Fiche d'un exercice de la catégorie : catégorie présélectionnée, enregistrement avec categoryId.
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 10)); h.spin(600)
    f = form(h)
    cr = F(h, "exerciseCategoryRow") if F(h, "exerciseCategoryRow") else None
    check(f is not None, f"{name} : fiche trouvée")
    opts = pv(f, "categoryOptions")
    texts = [o["text"] for o in opts]
    check(texts == ["U10-U11", "+ Nouvelle"], f"{name} : pastilles de la fiche {texts}")
    QMetaObject.invokeMethod(f, "save"); h.spin(200)
    check(len(B7.saved) == 1 and B7.saved[0].get("categoryId") == 1, f"{name} : enregistrement avec categoryId=1 ({B7.saved[-1:] })")
    check(B7.saved[0].get("category") == "technical", f"{name} : l'activité reste indépendante ({B7.saved[0].get('category')})")

    # Création d'une catégorie depuis la fiche, puis choix automatique.
    QMetaObject.invokeMethod(f, "createCategory", Q_ARG("QVariant", "U12-U13")); h.spin(200)
    check(B7.added == ["U12-U13"], f"{name} : addCategory appelé ({B7.added})")
    QMetaObject.invokeMethod(f, "save"); h.spin(200)
    check(B7.saved[-1].get("categoryId") == 3, f"{name} : la nouvelle catégorie est choisie à l'enregistrement ({B7.saved[-1].get('categoryId')})")
    p = page(h)
    check(len(pv(p, "categories")) == 2, f"{name} : la page recharge la liste des catégories ({len(pv(p, 'categories'))})")

    # Exercice sans catégorie : categoryId = 0.
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "exercises"), Q_ARG("QVariant", 12)); h.spin(600)
    f = form(h)
    QMetaObject.invokeMethod(f, "save"); h.spin(200)
    check(B7.saved[-1].get("id") == 12 and B7.saved[-1].get("categoryId") == 0, f"{name} : exercice sans catégorie -> categoryId=0 ({B7.saved[-1].get('categoryId')})")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
