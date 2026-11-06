# Effectif : import d'une liste de joueurs (écran, appels au backend simulé, erreurs par ligne, succès, état vide).
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
    calls = []
    outcome = {"ok": True, "created": 3, "error": "", "errors": []}
    @Slot(int, str, result="QVariantMap")
    def importPlayers(self, team, text):
        B4.calls.append((team, text)); return dict(B4.outcome)
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

from PySide6.QtCore import QObject
def walk(it, name, out):
    for c in it.childItems():
        if c.objectName() == name: out.append(c)
        walk(c, name, out)
    return out
def LE(dlg): return [c for c in walk(dlg.property("contentItem"), "importLineError", []) if c.isVisible()]   # délégués : pas des enfants QObject
def F(h, name, all_=False):
    out = [o for o in h.win.findChildren(QObject) if o.objectName() == name]
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


h = make(412, 892); pg = page(h) if F(h, "playerGrid") else None
imp = F(h, "importButton")
check(imp is not None and imp.isVisible(), "téléphone : bouton « importer » dans l'en-tête")
click_item(h, imp); h.spin(500)
dlg = F(h, "importDialog")
check(dlg.property("opened") is True, "le clic ouvre la fenêtre d'import")
check(dlg.property("teamId") == 1, "la fenêtre vise l'équipe affichée")
t = " ".join(texts(dlg.property("contentItem"), []))
check("Prénom ; Nom ; Numéro ; Poste ; Naissance" in t and "Seul le prénom est obligatoire" in t, "explication du format affichée")
check(not F(h, "importError").isVisible(), "aucune erreur à l'ouverture")
fld = F(h, "importList")
fld.setProperty("text", "Lucas\nThéo;Martin"); h.spin(100)

# Refus : message + lignes à corriger, la fenêtre reste ouverte et la saisie est conservée.
B4.calls = []
B4.outcome = {"ok": False, "created": 0, "error": "2 ligne(s) à corriger. Rien n'a été importé.",
              "errors": [{"line": 2, "message": "Théo Martin est déjà dans l'effectif de cette équipe."},
                         {"line": 5, "message": "Numéro « 100 » invalide (1 à 99)."}]}
click_item(h, F(h, "importSubmit")); h.spin(300)
check(B4.calls == [(1, "Lucas\nThéo;Martin")], f"le texte exact est transmis avec l'équipe : {B4.calls}")
check(F(h, "importError").isVisible() and "Rien n'a été importé" in F(h, "importError").property("text"), "message d'échec affiché")
le = LE(dlg)
check(sorted(c.property("text") for c in le) == ["Ligne 2 : Théo Martin est déjà dans l'effectif de cette équipe.", "Ligne 5 : Numéro « 100 » invalide (1 à 99)."],
      f"erreurs par ligne : {[c.property('text') for c in le]}")
check(dlg.property("opened") is True and fld.property("text") == "Lucas\nThéo;Martin", "fenêtre ouverte, saisie conservée")
shot(h, "import_erreurs")

# Beaucoup d'erreurs : 8 lignes au plus + résumé.
B4.outcome = {"ok": False, "created": 0, "error": "12 ligne(s) à corriger. Rien n'a été importé.",
              "errors": [{"line": i, "message": "x"} for i in range(1, 13)]}
click_item(h, F(h, "importSubmit")); h.spin(300)
check(len(LE(dlg)) == 8
      and any("… et 4 autre(s) ligne(s)." in x for x in texts(dlg.property("contentItem"), [])), "12 erreurs : 8 affichées puis « … et 4 autre(s) »")

# Succès : la fenêtre se ferme, la liste est rechargée, confirmation.
B4.outcome = {"ok": True, "created": 3, "error": "", "errors": []}
click_item(h, F(h, "importSubmit")); h.spin(500)
check(dlg.property("opened") is False, "succès : la fenêtre se ferme")
check(any("3 joueurs ajoutés." in x for x in texts(h.win.contentItem(), [])), "succès : confirmation « 3 joueurs ajoutés. »")

# Réouverture : vide, sans erreur.
click_item(h, F(h, "importButton")); h.spin(400)
check(F(h, "importList").property("text") == "" and not F(h, "importError").isVisible(), "réouverture : champ vide, sans erreur")

# Bureau et état vide.
h = make(1280, 800); check(F(h, "importButton").isVisible(), "bureau : bouton d'import présent")
h = make(412, 892, empty=True)
eb = F(h, "importEmptyButton")
check(eb is not None and eb.isVisible(), "équipe sans joueur : bouton « Importer une liste » dans l'état vide")
click_item(h, eb); h.spin(400)
check(F(h, "importDialog").property("opened") is True, "état vide : ouvre la fenêtre d'import")
h = make(412, 892, noteam=True)
check(not F(h, "importButton").isVisible(), "sans équipe : pas de bouton d'import")

print(f"\n{len(errs)} échec(s)"); sys.exit(1 if errs else 0)
