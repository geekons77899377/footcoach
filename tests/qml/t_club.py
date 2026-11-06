"""L4 : clubs — une équipe appartient toujours à un club ; création, renommage, suppression d'un club dans Effectif ;
choix du club à la création d'une équipe ; le club n'est rappelé dans les listes d'équipes que s'il y en a plusieurs.

Backend SIMULÉ : ce test valide l'interface QML, pas la base SQLite (voir tests/tst_matches.cpp).
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "t_live.py")).read().split("errs=[]")[0])
from PySide6.QtCore import QMetaObject, Q_ARG, Q_RETURN_ARG, QObject, Qt
errs = []
def check(c, msg):
    print(("OK  " if c else "KO  ") + msg)
    if not c: errs.append(msg)

class B7(B2):
    def __init__(self):
        super().__init__()
        self._club_init()
        self.teams_ = [{"id": 1, "clubId": 1, "name": "U15", "category": "U15", "season": "", "playerCount": 0}]
        self.saved_teams = []
        self.last_delete = None
    def _with_names(self):
        names = {c["id"]: c["name"] for c in self._clubs}
        return [dict(t, clubName=names.get(t["clubId"], "")) for t in self.teams_]
    @Slot(result="QVariantList")
    def teams(self): return self._with_names()
    @Slot("QVariantMap", result="QVariantMap")
    def saveTeam(self, t):
        self.saved_teams.append(dict(t))
        tid = max([x["id"] for x in self.teams_] + [0]) + 1
        club = int(t.get("clubId", -1)) if int(t.get("clubId", -1)) > 0 else self._clubs[0]["id"]
        self.teams_.append({"id": tid, "clubId": club, "name": t["name"], "category": t.get("category", ""), "season": t.get("season", ""), "playerCount": 0})
        return {"ok": True, "id": tid, "error": ""}
    @Slot(int, result="QVariantMap")
    def deleteClub(self, cid):
        r = Backend.deleteClub(self, cid)
        if r["ok"]: self.teams_ = [t for t in self.teams_ if t["clubId"] != cid]
        return r
    @Slot(int, bool, result="QVariantList")
    def players(self, team, inactive): return []
    @Slot(int, str, str, result="QStringList")
    def sessionDates(self, team, a, b): return []
    @Slot(int, str, str, result="QVariantList")
    def sessions(self, team, a, b): return []

def make():
    B7.mode = "none"; b = B7(); b.keep = []
    h = Harness(412, 892, backend=b)
    QMetaObject.invokeMethod(h.win, "go", Q_ARG("QVariant", "squad"), Q_ARG("QVariant", -1)); h.spin(600)
    return h, b
def find(h, pred):
    for o in h.win.findChildren(QObject):
        try:
            if pred(o): return o
        except Exception: pass
def cls(h, p): return find(h, lambda o: o.metaObject().className().startswith(p))
def jv(v): return v.toVariant() if hasattr(v, "toVariant") else v
def sheet_texts(h):
    sh = find(h, lambda o: o.metaObject().className().startswith("ActionSheet") and o.metaObject().indexOfMethod("rebuild()") >= 0)
    QMetaObject.invokeMethod(sh, "rebuild"); h.spin(100)
    return [m["text"] for m in jv(sh.property("model"))], sh
def choose(h, sh, id_):
    QMetaObject.invokeMethod(sh, "chosen", Q_ARG("QString", id_)); h.spin(400)

# --- 1. Un seul club : le nom du club n'encombre pas la liste ---------------------------------------------------------------------------
h, b = make()
texts, sh = sheet_texts(h)
check(not any("—" in t for t in texts), f"un seul club : aucune mention de club dans les équipes ({texts})")
check("Nouveau club" in texts and any(t.startswith("Renommer le club") for t in texts), "le menu propose Nouveau club / Renommer le club")
check(not any(t.startswith("Supprimer le club") for t in texts), "le dernier club ne peut pas être supprimé : pas d'entrée de suppression")

# --- 2. Nouveau club (saisie vide refusée côté backend) puis création -----------------------------------------------------------------------
prompt = find(h, lambda o: o.objectName() == "clubPrompt")
choose(h, sh, "newClub")
check(prompt.property("visible") is True, "« Nouveau club » ouvre la saisie du nom")
QMetaObject.invokeMethod(prompt, "accepted", Q_ARG("QString", "  ")); h.spin(300)
check(len(b._clubs) == 1, "nom de club vide : refusé, aucun club créé")
QMetaObject.invokeMethod(prompt, "accepted", Q_ARG("QString", "AS Voisins")); h.spin(300)
check([c["name"] for c in b._clubs] == ["Mon club", "AS Voisins"], "club créé")

# --- 3. Plusieurs clubs : le club est rappelé, suppression possible, choix du club à la création d'une équipe ---------------------------
texts, sh = sheet_texts(h)
check(any("— Mon club" in t for t in texts), f"plusieurs clubs : l'équipe est suivie de son club ({texts})")
check(any(t.startswith("Supprimer le club") for t in texts), "suppression du club proposée quand il y en a plusieurs")
dlg = cls(h, "TeamDialog")
QMetaObject.invokeMethod(dlg, "openFor", Q_ARG("QVariant", None), Q_ARG("QVariant", 1)); h.spin(400)
chips = find(h, lambda o: o.objectName() == "clubChips")
check(chips is not None and chips.property("visible") is True, "création d'équipe : le choix du club est proposé")
check(dlg.property("clubId") == 1, "le club de l'équipe courante est présélectionné")
QMetaObject.invokeMethod(chips, "picked", Q_ARG("QString", "2")); h.spin(200)
check(dlg.property("clubId") == 2, "choisir un autre club")
dlg.findChild(QObject, "") if False else None
# enregistrer : on déclenche le bouton « Enregistrer » via saveTeam directement depuis la boîte
name_field = [o for o in dlg.findChildren(QObject) if o.metaObject().className().startswith("FormField")][0]
name_field.setProperty("text", "U13")
btn = [o for o in dlg.findChildren(QObject) if o.metaObject().className().startswith("AppButton") and o.property("text") == "Enregistrer"][0]
QMetaObject.invokeMethod(btn, "clicked"); h.spin(500)
check(b.saved_teams and int(b.saved_teams[-1]["clubId"]) == 2, f"l'équipe est créée dans le club choisi ({b.saved_teams[-1:] })")
check(any(t["name"] == "U13" and t["clubId"] == 2 for t in b.teams_), "l'équipe U13 appartient au club AS Voisins")

# --- 4. Création d'équipe avec un seul club : pas de choix à faire -----------------------------------------------------------------------
h, b = make()
dlg = cls(h, "TeamDialog")
QMetaObject.invokeMethod(dlg, "openFor", Q_ARG("QVariant", None), Q_ARG("QVariant", 1)); h.spin(300)
chips = find(h, lambda o: o.objectName() == "clubChips")
check(chips is not None and chips.property("visible") is False, "un seul club : le choix du club est masqué")

# --- 5. Suppression d'un club : confirmation explicite, puis cascade ---------------------------------------------------------------------
h, b = make()
b.saveClub({"name": "AS Voisins"})
b.teams_.append({"id": 2, "clubId": 2, "name": "Seniors", "category": "", "season": "", "playerCount": 0})
page = cls(h, "SquadPage"); QMetaObject.invokeMethod(page, "refreshTeams"); h.spin(300)
page.setProperty("teamId", 2); h.spin(200)
texts, sh = sheet_texts(h)
choose(h, sh, "deleteClub")
conf = find(h, lambda o: o.metaObject().className().startswith("PromptDialog") and o.property("action") == "deleteClub")
check(conf is not None and conf.property("visible") is True, "suppression d'un club : confirmation demandée")
check("équipes" in conf.property("message") and "définitive" in conf.property("message"), "le message annonce la suppression des équipes et son caractère définitif")
check(len(b._clubs) == 2, "rien n'est supprimé avant la confirmation")
QMetaObject.invokeMethod(conf, "accepted", Q_ARG("QString", "")); h.spin(500)
check([c["id"] for c in b._clubs] == [1] and all(t["clubId"] == 1 for t in b.teams_), "après confirmation : le club et ses équipes sont supprimés")

print("\n%s" % ("ECHECS : %d" % len(errs) if errs else "TOUT PASSE"))
for e in errs: print("   KO ", e)
sys.exit(1 if errs else 0)
